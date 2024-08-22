#include "jit/jit.h"
#include "jit/jit_base.h"
#include "jit/jit_zicsr.h"
#include "jit/jit_a.h"
#include "jit/jit_m.h"
#include "jit/jit_f.h"
#include "jit/jit_c.h"
#include "opcodes_base.h"
#include "opcodes_m.h"
#include "opcodes_a.h"
#include "opcodes_f.h"
#include "opcodes_c.h"
#include "opcodes_zicsr.h"

using namespace JIT;

#define DEBUG_JIT false
#define FRAME_LIMIT 256

// Global CPU pointer for interface functions
static CPU* interface_cpu = nullptr;

// Cached previously translated code
static std::vector<Frame> cached_frames = {};

// Global LLVM
llvm::LLVMContext context;

// Hack to fix PC return issues
static bool csr_caused_tlb_flush = false;

#if DEBUG_JIT
llvm::Function* debug_trace;
llvm::Function* debug_print;
bool on_debug_trace(Instruction instruction, u64 pc);
void on_debug_print(u64 value);
#endif

void JIT::init()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

void JIT::run_next_frame(CPU& cpu)
{
    const u64 starting_pc = cpu.pc;

    // We cache in virtual address space so any changes to the TLB mean trouble
    if (cpu.tlb_was_flushed) [[unlikely]]
    {
        for (auto& frame : cached_frames)
            delete frame.engine;
        cached_frames.clear();
        cpu.tlb_was_flushed = false;
    }

    // Check if code has already been translated
    std::optional<Frame> frame = get_cached_frame(starting_pc);
    if (frame.has_value())
    {
        execute_frame(cpu, *frame, starting_pc);
    }
    else
    {
        frame = compile_next_frame(cpu);
        if (!frame.has_value())
        {
            // Some sort of exception occured when fetching the instruction
            // We will deal with it later but we must still raise it
            check_for_exceptions(cpu);
            return;
        }

        execute_frame(cpu, *frame, starting_pc);
        cache_frame(*frame);
    }
}

std::optional<Frame> JIT::compile_next_frame(CPU& cpu)
{
    // Create module
    llvm::Module* module = new llvm::Module("jit", context);
    llvm::IRBuilder builder(context);

    // Register functions
    Context jit_context(builder, context, cpu.pc);
    jit_context.registers = get_registers(cpu, builder);
    register_interface_functions(module, context, jit_context);

    // Create entry
    llvm::FunctionType* function_type = llvm::FunctionType::get(builder.getInt64Ty(), { builder.getInt64Ty() }, false);
    llvm::Function* function = llvm::Function::Create(
        function_type,
        llvm::Function::ExternalLinkage,
        "jit_main",
        module
    );
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entry);

    // We will be called with an argument corresponding to the PC, and use a switch
    // to jump to the correct label; populated later.
    llvm::Value* pc_arg = function->getArg(0);
#if DEBUG_JIT
    builder.CreateCall(debug_print, { pc_arg });
#endif
    llvm::BasicBlock* default_block = llvm::BasicBlock::Create(context, "default", function);
    llvm::SwitchInst* switch_instruction = builder.CreateSwitch(pc_arg, default_block, FRAME_LIMIT);
    std::unordered_map<u64, llvm::BasicBlock*> label_map;
    builder.SetInsertPoint(default_block);

    // Fetch and emit instructions...
    u64 starting_pc = cpu.pc;
    u64 ending_pc;
    bool frame_empty = true;
    for (int i = 0; i < FRAME_LIMIT; ++i)
    {
        // Check for 16-bit alignment
        if ((cpu.pc & 0b1) != 0)
        {
            cpu.raise_exception(Exception::InstructionAddressMisaligned, cpu.pc);
            delete module;
            return std::nullopt;
        }

        // Try to get a compressed instruction...
        const std::expected<CompressedInstruction, Exception> half_instruction =
            cpu.read_16(cpu.pc, CPU::AccessType::Instruction);
        const bool is_compressed = (half_instruction.has_value() && (half_instruction->instruction & 0b11) != 0b11);

        // ...or a full 32-bit one
        std::expected<Instruction, Exception> instruction = std::unexpected(Exception::IllegalInstruction);
        if (!is_compressed) instruction = cpu.read_32(cpu.pc, CPU::AccessType::Instruction);

        // If we couldn't fetch anything, raise an exception
        if (!is_compressed && !instruction)
        {
            if (frame_empty)
            {
                u64 faulty_address = cpu.pc;
                if (!cpu.read_8(cpu.pc)) faulty_address = cpu.pc;
                else if (!cpu.read_8(cpu.pc + 1)) faulty_address = cpu.pc + 1;
                else if (!cpu.read_8(cpu.pc + 1)) faulty_address = cpu.pc + 2;
                else faulty_address = cpu.pc + 3;

                cpu.raise_exception(instruction.error(), faulty_address);
                delete module;
                return std::nullopt;
            }
            else break;
        }

        // Check it's valid
        if (!is_compressed && (instruction->instruction == 0xffffffff || instruction->instruction == 0))
        {
            if (frame_empty)
            {
                cpu.raise_exception(Exception::IllegalInstruction, instruction->instruction);
                delete module;
                return std::nullopt;
            }
            else break;
        }
        else if (is_compressed && half_instruction->instruction == 0x0000)
        {
            if (frame_empty)
            {
                cpu.raise_exception(Exception::IllegalInstruction, half_instruction->instruction);
                delete module;
                return std::nullopt;
            }
            else break;
        }

        if (is_compressed)
            jit_context.current_compressed_instruction = *half_instruction;
        else
            jit_context.current_instruction = *instruction;

        llvm::BasicBlock* pc_block = llvm::BasicBlock::Create(context, "", function);
        builder.CreateBr(pc_block);
        builder.SetInsertPoint(pc_block);
        label_map[cpu.pc] = pc_block;

    #if DEBUG_JIT
        builder.CreateCall(debug_trace, {
            llvm::ConstantInt::get(builder.getInt32Ty(), jit_context.current_instruction.instruction),
            llvm::ConstantInt::get(builder.getInt64Ty(), jit_context.pc)
        });
    #endif

        if ((is_compressed && emit_compressed_instruction(cpu, jit_context)) ||
            (!is_compressed && emit_instruction(cpu, jit_context)))
        {
            // Now we've got at least one valid instruction, any failure to fetch
            // another might mean we just strayed too far into the future
            frame_empty = false;
        }
        else
        {
            if (frame_empty)
                throw std::runtime_error("JIT - unsupported opcode");
            else
                break;
        }

        ending_pc = cpu.pc;
        cpu.pc = jit_context.pc + (is_compressed ? 2 : 4);
        jit_context.pc = cpu.pc;

        if (jit_context.abort_translation)
            break;
    }

    // Return from block
    builder.CreateRet(llvm::ConstantInt::get(builder.getInt64Ty(), cpu.pc));

    // Fill in the switch instruction
    for (const auto& label : label_map)
    {
        llvm::BasicBlock* label_block = label.second;
        switch_instruction->addCase(llvm::ConstantInt::get(builder.getInt64Ty(), label.first), label_block);
    }

    // Build engine - TODO: fix tests that fail under optimisations so that they can occur??
    std::string error;
    llvm::ExecutionEngine* engine = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module))
        .setErrorStr(&error)
        .setOptLevel(llvm::CodeGenOptLevel::None)
        .setEngineKind(llvm::EngineKind::JIT)
        .create();

    if (!engine)
        throw std::runtime_error("failed to create llvm::ExecutionEngine: " + error);

    link_interface_functions(engine, jit_context);

#if DEBUG_JIT
    if (starting_pc == 0x80E77050)
        module->print(llvm::outs(), nullptr);

    assert(!llvm::verifyModule(*module, &llvm::errs()));
    assert(!llvm::verifyFunction(*function, &llvm::errs()));
#endif

    // IR will be lazily compiled when we call getFunctionAddress but it's nicer
    // to do it here so it makes more sense in the profiler
    engine->finalizeObject();

    return Frame(engine, starting_pc, ending_pc);
}

void JIT::execute_frame(CPU& cpu, Frame& frame, u64 pc)
{
    // Run
    interface_cpu = &cpu;
    auto run = (u64(*)(u64))frame.engine->getFunctionAddress("jit_main");
    u64 next_pc = run(pc);
    cpu.pc = next_pc;

    if (!check_for_exceptions(cpu))
        return;

    if (csr_caused_tlb_flush)
    {
        cpu.pc += 4;
        csr_caused_tlb_flush = false;
    }

    if (cpu.tlb_was_flushed)
        return;

    // If the next PC is inside the already JIT'ed block, we can instead just jump back
    while(next_pc >= frame.starting_pc && next_pc <= frame.ending_pc)
    {
        next_pc = run(next_pc);
        cpu.pc = next_pc;

        if (!check_for_exceptions(cpu))
            return;

        if (csr_caused_tlb_flush)
        {
            cpu.pc += 4;
            csr_caused_tlb_flush = false;
        }

        if (cpu.tlb_was_flushed)
            return;
    }
}

void JIT::cache_frame(Frame& frame)
{
    cached_frames.emplace_back(frame);
}

std::optional<Frame> JIT::get_cached_frame(u64 pc)
{
    for (const auto& frame : cached_frames)
    {
        if (pc >= frame.starting_pc && pc <= frame.ending_pc)
            return frame;
    }

    return std::nullopt;
}

bool JIT::check_for_exceptions(CPU& cpu)
{
    const std::optional<CPU::PendingTrap> trap = cpu.get_pending_trap();
    if (trap.has_value())
    {
        cpu.handle_trap(trap->cause, trap->info, trap->is_interrupt);
        return false;
    }
    return true;
}

bool JIT::emit_instruction(CPU& cpu, Context& context)
{
    const u8 opcode = context.current_instruction.get_opcode();
    const u8 funct3 = context.current_instruction.get_funct3();
    const u8 funct7 = context.current_instruction.get_funct7();
    const u8 funct2 = context.current_instruction.get_funct2();

    switch (opcode)
    {
        case OPCODES_BASE_R_TYPE:
        {
            // RV64M
            if (funct7 == OPCODES_M_FUNCT_7)
            {
                switch (funct3)
                {
                    case MUL:       mul   (context); break;
                    case MULH:      mulh  (context); break;
                    case MULHSU:    mulhsu(context); break;
                    case MULHU:     mulhu (context); break;
                    case DIV:       div   (context); break;
                    case DIVU:      divu  (context); break;
                    case REM:       rem   (context); break;
                    case REMU:      remu  (context); break;
                    default:        return false;
                }
                break;
            }

            switch (funct3)
            {
                case ADD:
                {
                    switch (funct7)
                    {
                        case 0:   add(context); break;
                        case SUB: sub(context); break;
                        default:  return false;
                    }
                    break;
                }
                case XOR: _xor(context); break;
                case OR:  _or (context); break;
                case AND: _and(context); break;
                case SLL:  sll(context); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7)
                    {
                        case SRL: srl(context); break;
                        case SRA: sra(context); break;
                        default:  return false;
                    }
                    break;
                }

                case SLT:  slt (context); break;
                case SLTU: sltu(context); break;
                default:   return false;
            }
            break;
        }

        case OPCODES_BASE_I_TYPE:
        {
            switch (funct3)
            {
                case ADDI: addi(context); break;
                case XORI: xori(context); break;
                case ORI:  ori (context); break;
                case ANDI: andi(context); break;
                case SLLI: slli(context); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7 & 0b11111110)
                    {
                        case SRAI: srai(context); break;
                        case 0:    srli(context); break;
                        default:   return false;
                    }
                    break;
                }

                case SLTI:  slti (context); break;
                case SLTIU: sltiu(context); break;
                default:    return false;
            }
            break;
        }

        case OPCODES_BASE_LOAD_TYPE:
        {
            switch (funct3)
            {
                case LB:    lb (context); break;
                case LH:    lh (context); break;
                case LW:    lw (context); break;
                case LBU:   lbu(context); break;
                case LHU:   lhu(context); break;

                // RV64I-specific
                case LWU:   lwu(context); break;
                case LD:    ld (context); break;
                default:    return false;
            }
            break;
        }

        case OPCODES_BASE_S_TYPE:
        {
            switch (funct3)
            {
                case SB:    sb(context); break;
                case SH:    sh(context); break;
                case SW:    sw(context); break;
                case SD:    sd(context); break; // RV64I
                default:    return false;
            }
            break;
        }

        case OPCODES_BASE_B_TYPE:
        {
            switch (funct3)
            {
                case BEQ:   beq (context); break;
                case BNE:   bne (context); break;
                case BLT:   blt (context); break;
                case BGE:   bge (context); break;
                case BLTU:  bltu(context); break;
                case BGEU:  bgeu(context); break;
                default:    return false;
            }
            break;
        }

        case JAL:   jal  (context); break;
        case JALR:  jalr (context); break;
        case LUI:   lui  (context); break;
        case AUIPC: auipc(context); break;

        case OPCODES_BASE_SYSTEM:
        {
            // Zicsr instructions
            if (funct3 != 0)
            {
                switch (funct3)
                {
                    case CSRRW:     csrrw (context); break;
                    case CSRRS:     csrrs (context); break;
                    case CSRRC:     csrrc (context); break;
                    case CSRRWI:    csrrwi(context); break;
                    case CSRRSI:    csrrsi(context); break;
                    case CSRRCI:    csrrci(context); break;

                    default:
                        return false;
                }
                break;
            }

            const u8 rs2 = context.current_instruction.get_rs2();

            if (rs2 == ECALL && funct7 == 0)
            {
                ecall(context);
                return true;
            }

            if (rs2 == EBREAK && funct7 == 0)
            {
                ebreak(context);
                return true;
            }

            if (rs2 == URET && funct7 == 0)
            {
                uret(context);
                return true;
            }

            if (rs2 == 2 && funct7 == SRET)
            {
                sret(context);
                return true;
            }

            if (rs2 == 2 && funct7 == MRET)
            {
                mret(context);
                return true;
            }

            if (rs2 == 5 && funct7 == WFI)
            {
                wfi(context);
                return true;
            }

            if (funct7 == SFENCE_VMA)
            {
                sfence_vma(context);
                return true;
            }

            if (funct7 == HFENCE_BVMA) throw std::runtime_error("hfence.bvma");
            if (funct7 == HFENCE_GVMA) throw std::runtime_error("hfence.gvma");

            return false;
        }

        case OPCODES_BASE_FENCE:
        {
            // No cores; no need!
            break;
        }

        case OPCODES_BASE_I_TYPE_32:
        {
            switch (funct3)
            {
                case ADDIW: addiw(context); break;
                case SLLIW: slliw(context); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7)
                    {
                        case SRLIW: srliw(context); break;
                        case SRAIW: sraiw(context); break;
                        default:    return false;
                    }
                    break;
                }

                default: return false;
            }
            break;
        }

        case OPCODES_BASE_R_TYPE_32:
        {
            // RV64M
            if (funct7 == OPCODES_M_FUNCT_7)
            {
                switch (funct3)
                {
                    case MULW:      mulw (context);  break;
                    case DIVW:      divw (context);  break;
                    case DIVUW:     divuw(context);  break;
                    case REMW:      remw (context);  break;
                    case REMUW:     remuw(context);  break;
                    default:        return false;
                }
                break;
            }

            switch (funct3)
            {
                case ADDW:
                {
                    switch (funct7)
                    {
                        case ADDW:  addw(context); break;
                        case SUBW:  subw(context); break;
                        default:    return false;
                    }
                    break;
                }

                case SLLW: sllw(context); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7)
                    {
                        case SRLW:  srlw(context); break;
                        case SRAW:  sraw(context); break;
                        default:    return false;
                    }
                    break;
                }

                default: return false;
            }
            break;
        }

        // RV64A
        case OPCODES_A:
        {
            switch (funct3)
            {
                case OPCODES_A_FUNCT_3:
                {
                    switch (funct7 >> 2)
                    {
                        case LR_W:      lr_w     (context); break;
                        case SC_W:      sc_w     (context); break;
                        case AMOSWAP_W: amoswap_w(context); break;
                        case AMOADD_W:  amoadd_w (context); break;
                        case AMOXOR_W:  amoxor_w (context); break;
                        case AMOAND_W:  amoand_w (context); break;
                        case AMOOR_W:   amoor_w  (context); break;
                        case AMOMIN_W:  amomin_w (context); break;
                        case AMOMAX_W:  amomax_w (context); break;
                        case AMOMINU_W: amominu_w(context); break;
                        case AMOMAXU_W: amomaxu_w(context); break;
                        default:        return false;
                    }
                    break;
                }

                case OPCODES_A_64:
                {
                    switch (funct7 >> 2)
                    {
                        case LR_D:      lr_d     (context); break;
                        case SC_D:      sc_d     (context); break;
                        case AMOSWAP_D: amoswap_d(context); break;
                        case AMOADD_D:  amoadd_d (context); break;
                        case AMOXOR_D:  amoxor_d (context); break;
                        case AMOAND_D:  amoand_d (context); break;
                        case AMOOR_D:   amoor_d  (context); break;
                        case AMOMIN_D:  amomin_d (context); break;
                        case AMOMAX_D:  amomax_d (context); break;
                        case AMOMINU_D: amominu_d(context); break;
                        case AMOMAXU_D: amomaxu_d(context); break;
                        default:        return false;
                    }
                    break;
                }

                default: return false;
            }
            break;
        }

        // RV64FD
        case OPCODES_F_1:
        {
            switch (funct3)
            {
                case FLW: flw(context); return true;;
                case FLD: fld(context); return true;;
                default: return false;
            }
        }

        case OPCODES_F_2:
        {
            switch (funct3)
            {
                case FSW: fsw(context); return true;;
                case FSD: fsd(context); return true;;
                default: return false;
            }
        }

        case OPCODES_F_3:
        {
            switch (funct2)
            {
                case FMADD_S: fmadd_s(context); return true;;
                case FMADD_D: fmadd_d(context); return true;;
                default: return false;
            }
        }

        case OPCODES_F_4:
        {
            switch (funct2)
            {
                case FMSUB_S: fmsub_s(context); return true;;
                case FMSUB_D: fmsub_d(context); return true;;
                default: return false;
            }
        }

        case OPCODES_F_5:
        {
            switch (funct2)
            {
                case FNMADD_S: fnmadd_s(context); return true;;
                case FNMADD_D: fnmadd_d(context); return true;;
                default: return false;
            }
        }

        case OPCODES_F_6:
        {
            switch (funct2)
            {
                case FNMSUB_S: fnmsub_s(context); return true;;
                case FNMSUB_D: fnmsub_d(context); return true;;
                default: return false;
            }
        }

        case OPCODES_F_7:
        {
            switch (funct7)
            {
                case FADD_S: fadd_s(context); return true;;
                case FADD_D: fadd_d(context); return true;;
                case FSUB_S: fsub_s(context); return true;;
                case FSUB_D: fsub_d(context); return true;;
                case FMUL_S: fmul_s(context); return true;;
                case FMUL_D: fmul_d(context); return true;;
                case FDIV_S: fdiv_s(context); return true;;
                case FDIV_D: fdiv_d(context); return true;;

                case 0x10:
                {
                    switch (funct3)
                    {
                        case FSGNJ_S:  fsgnj_s(context);  return true;
                        case FSGNJN_S: fsgnjn_s(context); return true;;
                        case FSGNJX_S: fsgnjx_s(context); return true;;
                        default: return false;
                    }
                }

                case 0x11:
                {
                    switch (funct3)
                    {
                        case FSGNJ_D:  fsgnj_d(context);  return true;
                        case FSGNJN_D: fsgnjn_d(context); return true;;
                        case FSGNJX_D: fsgnjx_d(context); return true;;
                        default: return false;
                    }
                }

                case 0x14:
                {
                    switch (funct3)
                    {
                        case FMIN_S: fmin_s(context); return true;;
                        case FMAX_S: fmax_s(context); return true;;
                        default: return false;
                    }
                }

                case 0x15:
                {
                    switch (funct3)
                    {
                        case FMIN_D: fmin_d(context); return true;;
                        case FMAX_D: fmax_d(context); return true;;
                        default: return false;
                    }
                }

                case 0x50:
                {
                    switch (funct3)
                    {
                        case FEQ_S: feq_s(context); return true;;
                        case FLT_S: flt_s(context); return true;;
                        case FLE_S: fle_s(context); return true;;
                        default: return false;
                    }
                }

                case 0x51:
                {
                    switch (funct3)
                    {
                        case FEQ_D: feq_d(context); return true;;
                        case FLT_D: flt_d(context); return true;;
                        case FLE_D: fle_d(context); return true;;
                        default: return false;
                    }
                }

                case 0x60:
                {
                    switch (context.current_instruction.get_rs2())
                    {
                        case FCVT_W_S:  fcvt_w_s(context);  return true;
                        case FCVT_L_S:  fcvt_l_s(context);  return true;
                        case FCVT_WU_S: fcvt_wu_s(context); return true;;
                        case FCVT_LU_S: fcvt_lu_s(context); return true;;
                        default: return false;
                    }
                }

                case 0x61:
                {
                    switch (context.current_instruction.get_rs2())
                    {
                        case FCVT_W_D:  fcvt_w_d(context);  return true;
                        case FCVT_L_D:  fcvt_l_d(context);  return true;
                        case FCVT_WU_D: fcvt_wu_d(context); return true;;
                        case FCVT_LU_D: fcvt_lu_d(context); return true;;
                        default: return false;
                    }
                }

                case 0x68:
                {
                    switch (context.current_instruction.get_rs2())
                    {
                        case FCVT_S_W:  fcvt_s_w(context);  return true;
                        case FCVT_S_L:  fcvt_s_l(context);  return true;
                        case FCVT_S_WU: fcvt_s_wu(context); return true;;
                        case FCVT_S_LU: fcvt_s_lu(context); return true;;
                        default: return false;
                    }
                }

                case 0x69:
                {
                    switch (context.current_instruction.get_rs2())
                    {
                        case FCVT_D_W:  fcvt_d_w(context);  return true;
                        case FCVT_D_L:  fcvt_d_l(context);  return true;
                        case FCVT_D_WU: fcvt_d_wu(context); return true;;
                        case FCVT_D_LU: fcvt_d_lu(context); return true;;
                        default: return false;
                    }
                }

                case 0x70:
                {
                    switch (funct3)
                    {
                        case FMV_X_W:  fmv_x_w(context);  return true;
                        case FCLASS_S: fclass_s(context); return true;;
                        default: return false;
                    }
                }

                case 0x71:
                {
                    switch (funct3)
                    {
                        case FMV_X_D:  fmv_x_d(context);  return true;
                        case FCLASS_D: fclass_d(context); return true;;
                        default: return false;
                    }
                }

                case FCVT_S_D: fcvt_s_d(context); return true;;
                case FCVT_D_S: fcvt_d_s(context); return true;;
                case FSQRT_S:  fsqrt_s(context);  return true;
                case FSQRT_D:  fsqrt_d(context);  return true;
                case FMV_W_X:  fmv_w_x(context);  return true;
                case FMV_D_X:  fmv_d_x(context);  return true;
                default: return false;
            }
        }

        default:
            return false;
    }

    return true;
}

bool JIT::emit_compressed_instruction(CPU& cpu, Context& context)
{
    const u8 funct3 = context.current_compressed_instruction.get_funct3();
    const u8 opcode = context.current_compressed_instruction.get_opcode();

    switch (opcode)
    {
        case 0b00:
        {
            switch (funct3)
            {
                case C_LW:          c_lw(context);         return true;
                case C_LD:          c_ld(context);         return true;
                case C_SW:          c_sw(context);         return true;
                case C_SD:          c_sd(context);         return true;
                case C_ADDI4SPN:    c_addi4spn(context);   return true;
                case C_FLD:         c_fld(context);        return true;
                case C_FSD:         c_fsd(context);        return true;
                default:                                            return false;
            }

            return false;
        }

        case 0b01:
        {
            switch (funct3)
            {
                case C_LI:          c_li(context);         return true;
                case C_J:           c_j(context);          return true;
                case C_BEQZ:        c_beqz(context);       return true;
                case C_BNEZ:        c_bnez(context);       return true;
                case C_ADDI:        c_addi(context);       return true;
                case C_ADDIW:       c_addiw(context);      return true;
                case C_ADDI16SP:
                {
                    switch (context.current_compressed_instruction.get_rd())
                    {
                        case 0:                                     return true; // NOP
                        case 2:     c_addi16sp(context);   return true;
                        default:    c_lui(context);        return true;
                    }
                }
                case 0b100:
                {
                    switch (context.current_compressed_instruction.get_funct2())
                    {
                        case C_SRLI: c_srli(context);      return true;
                        case C_SRAI: c_srai(context);      return true;
                        case C_ANDI: c_andi(context);      return true;
                        case 0b11:
                        {
                            const u8 a = (context.current_compressed_instruction.instruction >> 12) & 0b1;
                            const u8 b = (context.current_compressed_instruction.instruction >> 5) & 0b11;

                            if (a == 0 && b == 0)
                            {
                                c_sub(context);
                                return true;
                            }

                            if (a == 0 && b == 1)
                            {
                                c_xor(context);
                                return true;
                            }

                            if (a == 0 && b == 2)
                            {
                                c_or(context);
                                return true;
                            }

                            if (a == 0 && b == 3)
                            {
                                c_and(context);
                                return true;
                            }

                            if (a == 1 && b == 0)
                            {
                                c_subw(context);
                                return true;
                            }

                            if (a == 1 && b == 1)
                            {
                                c_addw(context);
                                return true;
                            }

                            return false;
                        }
                        default: return false;
                    }
                }
                default: return false;
            }

            return false;
        }

        case 0b10:
        {
            switch (funct3)
            {
                case C_LWSP:  c_lwsp(context);  return true;
                case C_LDSP:  c_ldsp(context);  return true;
                case C_FLDSP: c_fldsp(context); return true;
                case C_SLLI:  c_slli(context);  return true;

                case 0b100:
                {
                    const u8 a = (context.current_compressed_instruction.instruction >> 12) & 0b1;
                    const u8 b = (context.current_compressed_instruction.instruction >> 2) & 0x1f;

                    if (a == 0 && b == 0)
                    {
                        c_jr(context);
                        return true;
                    }

                    else if (a == 0)
                    {
                        c_mv(context);
                        return true;
                    }

                    if (a == 1 && b == 0)
                    {
                        if (context.current_compressed_instruction.get_rd() != 0)
                        {
                            c_jalr(context);
                            return true;
                        }
                        else
                        {
                            c_ebreak(context);
                            return true;
                        }

                        return false;
                    }

                    if (a == 1)
                    {
                        c_add(context);
                        return true;
                    }

                    return false;
                }

                case C_SWSP:  c_swsp(context);  return true;
                case C_SDSP:  c_sdsp(context);  return true;
                case C_FSDSP: c_fsdsp(context); return true;
                default: return false;
            }

            return false;
        }
        default: return false;
    }

    return false;
}

llvm::Value* JIT::get_registers(CPU& cpu, llvm::IRBuilder<>& builder)
{
    llvm::Type* i64_ptr_type = builder.getInt64Ty()->getPointerTo();
    return builder.CreateIntToPtr(
        llvm::ConstantInt::get(builder.getInt64Ty(), reinterpret_cast<uint64_t>(cpu.registers)),
        i64_ptr_type
    );
}

llvm::Value* JIT::load_register(Context& context, u32 index)
{
#ifdef DEBUG_JIT
    assert(index <= 31);
#endif

    // x0 is always zero
    if (index == 0)
        return llvm::ConstantInt::get(context.builder.getInt64Ty(), 0);

    llvm::Value* index_value = llvm::ConstantInt::get(context.builder.getInt32Ty(), index);
    llvm::Value* element_pointer = context.builder.CreateGEP(
        context.builder.getInt64Ty(),
        context.registers,
        index_value
    );
    return context.builder.CreateLoad(context.builder.getInt64Ty(), element_pointer);
}

void JIT::store_register(Context& context, u32 index, llvm::Value* value)
{
#ifdef DEBUG_JIT
    assert(index <= 31);
#endif

    // x0 is always zero
    if (index == 0)
        return;

    llvm::Value* index_value = llvm::ConstantInt::get(context.builder.getInt32Ty(), index);
    llvm::Value* element_pointer = context.builder.CreateGEP(
        context.builder.getInt64Ty(),
        context.registers,
        index_value
    );

    context.builder.CreateStore(value, element_pointer);
}

/*
    The following on_* opcodes return the next program counter.
    Unfortunately, should an exception occur (which for some opcodes
    is all the time), the PC will be 2 or 4 too high as a result, meaning
    the exception's info field might be wrong. Hence care should be taken
    to return the PC of the next valid instruction, but *only* *if no
    exception occurred! The interpeter does this too, but for every opcode.
*/
#define RETURN_FROM_OPCODE_HANDLER(x)\
    if (interface_cpu->pending_trap.has_value())\
        return interface_cpu->pc;\
    else\
        return interface_cpu->pc + x;

u64 on_ecall(u64 pc)
{
    interface_cpu->pc = pc;
    ::ecall(*interface_cpu, Instruction(0));
    RETURN_FROM_OPCODE_HANDLER(4);
}

u64 on_ebreak(u64 pc)
{
    interface_cpu->pc = pc;
    ::ebreak(*interface_cpu, Instruction(0));
    RETURN_FROM_OPCODE_HANDLER(4);
}

u64 on_c_ebreak(u64 pc)
{
    interface_cpu->pc = pc;
    ::ebreak(*interface_cpu, Instruction(0));
    RETURN_FROM_OPCODE_HANDLER(2);
}

u64 on_uret(u64 pc)
{
    interface_cpu->pc = pc;
    ::uret(*interface_cpu, Instruction(0));
    RETURN_FROM_OPCODE_HANDLER(4);
}

u64 on_sret(u64 pc)
{
    interface_cpu->pc = pc;
    ::sret(*interface_cpu, Instruction(0));
    RETURN_FROM_OPCODE_HANDLER(4);
}

u64 on_mret(u64 pc)
{
    interface_cpu->pc = pc;
    ::mret(*interface_cpu, Instruction(0));
    RETURN_FROM_OPCODE_HANDLER(4);
}

u64 on_wfi(u64 pc)
{
    interface_cpu->pc = pc;
    ::wfi(*interface_cpu, Instruction(0));
    RETURN_FROM_OPCODE_HANDLER(4);
}

u64 on_sfence_vma(u64 pc)
{
    // sfence.vma doesn't care about the instruction currently but might in
    // the future when we support partial TLB invalidations
    interface_cpu->pc = pc;
    ::sfence_vma(*interface_cpu, Instruction(0));
    RETURN_FROM_OPCODE_HANDLER(4);
}

/*
    For some reason, Clang will fail to properly execute loads when optimisations
    are enabled. TODO: fix what is likely undefined behaviour
 */
#ifdef __clang__
#pragma clang optimize off
#endif

template<auto F, typename T>
T on_load(u64 address, u64 pc, bool* did_succeed)
{
    interface_cpu->pc = pc;
    const auto value = (interface_cpu->*F)(address, CPU::AccessType::Load);
    if (!value)
    {
        interface_cpu->raise_exception(value.error());
        *did_succeed = false;
        return 0;
    }

    *did_succeed = true;
    return *value;
}

#ifdef __clang__
#pragma clang optimize on
#endif

u8 on_lb(u64 address, u64 pc, bool* did_succeed)
{
    return on_load<&CPU::read_8, u8>(address, pc, did_succeed);
}

u16 on_lh(u64 address, u64 pc, bool* did_succeed)
{
    return on_load<&CPU::read_16, u16>(address, pc, did_succeed);
}

u32 on_lw(u64 address, u64 pc, bool* did_succeed)
{
    return on_load<&CPU::read_32, u32>(address, pc, did_succeed);
}

u32 on_ld(u64 address, u64 pc, bool* did_succeed)
{
    return on_load<&CPU::read_64, u64>(address, pc, did_succeed);
}

template<auto F, typename T>
bool on_store(u64 address, T value, u64 pc)
{
    interface_cpu->pc = pc;
    const auto error = (interface_cpu->*F)(address, value, CPU::AccessType::Store);
    if (error.has_value())
    {
        interface_cpu->raise_exception(*error);
        return false;
    }
    return true;
}

bool on_sb(u64 address, u8 value, u64 pc)
{
    return on_store<&CPU::write_8, u8>(address, value, pc);
}

bool on_sh(u64 address, u16 value, u64 pc)
{
    return on_store<&CPU::write_16, u16>(address, value, pc);
}

bool on_sw(u64 address, u32 value, u64 pc)
{
    return on_store<&CPU::write_32, u32>(address, value, pc);
}

bool on_sd(u64 address, u64 value, u64 pc)
{
    return on_store<&CPU::write_64, u64>(address, value, pc);
}

bool on_csr(Instruction instruction, u64 pc)
{
    interface_cpu->pc = pc;
    ::opcodes_zicsr(*interface_cpu, instruction);
    interface_cpu->check_for_invalid_tlb();
    interface_cpu->registers[0] = 0;

    // Return if an exception occured or the TLB was invalidated
    // All the PC logic goes to great lengths to preserve the PC
    // when we return false so we'll have to be a bit creative
    csr_caused_tlb_flush = interface_cpu->tlb_was_flushed;
    return !interface_cpu->pending_trap.has_value() && !interface_cpu->tlb_was_flushed;
}

void set_fcsr_dz()
{
    interface_cpu->fcsr.set_dz(*interface_cpu);
}

bool on_atomic(Instruction instruction, u64 pc)
{
    interface_cpu->pc = pc;
    ::opcodes_a(*interface_cpu, instruction);
    interface_cpu->registers[0] = 0;
    return !interface_cpu->pending_trap.has_value();
}

bool on_floating(Instruction instruction, u64 pc)
{
    interface_cpu->pc = pc;
    ::opcodes_f(*interface_cpu, instruction);
    interface_cpu->registers[0] = 0;
    return !interface_cpu->pending_trap.has_value();
}

bool on_floating_compressed(CompressedInstruction instruction, u64 pc)
{
    interface_cpu->pc = pc;
    ::opcodes_c(*interface_cpu, instruction);
    interface_cpu->registers[0] = 0;
    return !interface_cpu->pending_trap.has_value();
}

#if DEBUG_JIT
bool on_debug_trace(Instruction instruction, u64 pc)
{
    // TODO: make separate stub for compressed instructions
    char buf[80] = { 0 };
    disasm_inst(
        buf,
        sizeof(buf),
        rv64,
        pc,
        instruction.instruction
    );
    printf("%016" PRIx64 ":  %s\n", pc, buf);
    return true;
}

void on_debug_print(u64 value)
{
    (void)value;
}
#endif

void JIT::register_interface_functions(
    llvm::Module* module,
    llvm::LLVMContext& context,
    Context& jit_context
)
{
    llvm::FunctionType* fallback_type = llvm::FunctionType::get
    (
        llvm::Type::getInt1Ty(context),
        {
            llvm::Type::getInt32Ty(context),
            llvm::Type::getInt64Ty(context)
        },
        false
    );

    llvm::FunctionType* fallback_compressed_type = llvm::FunctionType::get
    (
        llvm::Type::getInt1Ty(context),
        {
            llvm::Type::getInt16Ty(context),
            llvm::Type::getInt64Ty(context)
        },
        false
    );

    #define TWINE_NAME(name) #name

    #define FALLBACK(name)\
        jit_context.name = llvm::Function::Create(\
            fallback_type,\
            llvm::Function::ExternalLinkage,\
            TWINE_NAME(name),\
            module\
        );

    #define FALLBACK_COMPRESSED(name)\
        jit_context.name = llvm::Function::Create(\
            fallback_compressed_type,\
            llvm::Function::ExternalLinkage,\
            TWINE_NAME(name),\
            module\
        );

    #define OPCODE_TYPE_1(return_type)\
        llvm::FunctionType::get\
        (\
            return_type,\
            { llvm::Type::getInt64Ty(context) },\
            false\
        )

    #define OPCODE_TYPE_2(return_type)\
        llvm::FunctionType::get\
        (\
            return_type,\
            {\
                llvm::Type::getInt64Ty(context),\
                llvm::Type::getInt64Ty(context),\
                llvm::PointerType::get(llvm::Type::getInt1Ty(context), 0)\
            },\
            false\
        )

    #define OPCODE_TYPE_3(data_type)\
        llvm::FunctionType::get\
        (\
            llvm::Type::getInt1Ty(context),\
            {\
                llvm::Type::getInt64Ty(context),\
                data_type,\
                llvm::Type::getInt64Ty(context)\
            },\
            false\
        )

    #define OPCODE_TYPE_4()\
        llvm::FunctionType::get\
        (\
            llvm::Type::getVoidTy(context),\
            {},\
            false\
        )

    #define OPCODE(name, return_type)\
        jit_context.name = llvm::Function::Create(\
            return_type,\
            llvm::Function::ExternalLinkage,\
            TWINE_NAME(name),\
            module\
        )

    OPCODE(on_ecall,        OPCODE_TYPE_1(llvm::Type::getInt64Ty(context)));
    OPCODE(on_ebreak,       OPCODE_TYPE_1(llvm::Type::getInt64Ty(context)));
    OPCODE(on_c_ebreak,     OPCODE_TYPE_1(llvm::Type::getInt64Ty(context)));
    OPCODE(on_uret,         OPCODE_TYPE_1(llvm::Type::getInt64Ty(context)));
    OPCODE(on_sret,         OPCODE_TYPE_1(llvm::Type::getInt64Ty(context)));
    OPCODE(on_mret,         OPCODE_TYPE_1(llvm::Type::getInt64Ty(context)));
    OPCODE(on_wfi,          OPCODE_TYPE_1(llvm::Type::getInt64Ty(context)));
    OPCODE(on_sfence_vma,   OPCODE_TYPE_1(llvm::Type::getInt64Ty(context)));
    OPCODE(on_lb,           OPCODE_TYPE_2(llvm::Type::getInt8Ty(context)));
    OPCODE(on_lh,           OPCODE_TYPE_2(llvm::Type::getInt16Ty(context)));
    OPCODE(on_lw,           OPCODE_TYPE_2(llvm::Type::getInt32Ty(context)));
    OPCODE(on_ld,           OPCODE_TYPE_2(llvm::Type::getInt64Ty(context)));
    OPCODE(on_sb,           OPCODE_TYPE_3(llvm::Type::getInt8Ty(context)));
    OPCODE(on_sh,           OPCODE_TYPE_3(llvm::Type::getInt16Ty(context)));
    OPCODE(on_sw,           OPCODE_TYPE_3(llvm::Type::getInt32Ty(context)));
    OPCODE(on_sd,           OPCODE_TYPE_3(llvm::Type::getInt64Ty(context)));
    OPCODE(set_fcsr_dz,     OPCODE_TYPE_4());

    FALLBACK(on_csr);
    FALLBACK(on_atomic);
    FALLBACK(on_floating);
    FALLBACK_COMPRESSED(on_floating_compressed);

#if DEBUG_JIT
    debug_trace = llvm::Function::Create(
        fallback_type,
        llvm::Function::ExternalLinkage,
        "debug_trace",
        module
    );
    debug_print = llvm::Function::Create(
        llvm::FunctionType::get
        (
            llvm::Type::getVoidTy(context),
            { llvm::Type::getInt64Ty(context) },
            false
        ),
        llvm::Function::ExternalLinkage,
        "debug_print",
        module
    );
#endif
}

void JIT::link_interface_functions(
    llvm::ExecutionEngine* engine,
    Context& jit_context
)
{
    #define LINK(name)\
        engine->addGlobalMapping(jit_context.name, (void*)&name);

    LINK(on_ecall);
    LINK(on_ebreak);
    LINK(on_c_ebreak);
    LINK(on_uret);
    LINK(on_sret);
    LINK(on_mret);
    LINK(on_wfi);
    LINK(on_sfence_vma);
    LINK(on_lb);
    LINK(on_lh);
    LINK(on_lw);
    LINK(on_ld);
    LINK(on_sb);
    LINK(on_sh);
    LINK(on_sw);
    LINK(on_sd);
    LINK(set_fcsr_dz);

    LINK(on_csr);
    LINK(on_atomic);
    LINK(on_floating);
    LINK(on_floating_compressed);

#if DEBUG_JIT
    engine->addGlobalMapping(debug_trace, (void*)&on_debug_trace);
    engine->addGlobalMapping(debug_print, (void*)&on_debug_print);
#endif
}
