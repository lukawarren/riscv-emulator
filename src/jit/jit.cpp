#include "jit/jit.h"
#include "jit/jit_base.h"
#include "jit/jit_zicsr.h"
#include "jit/jit_a.h"
#include "opcodes_base.h"
#include "opcodes_m.h"
#include "opcodes_a.h"
#include "opcodes_zicsr.h"

using namespace JIT;

#define DEBUG_JIT true

// Global CPU pointer (for this file only) for said interface functions
static CPU* interface_cpu = nullptr;

void JIT::init()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

void JIT::run_next_frame(CPU& cpu)
{
    // Init LLVM
    llvm::LLVMContext context;
    llvm::Module* module = new llvm::Module("jit", context);
    llvm::IRBuilder builder(context);

    // Register functions
    Context jit_context(builder, context, cpu.pc);
    jit_context.registers = get_registers(cpu, builder);
    register_interface_functions(module, context, jit_context);

    // Create entry
    llvm::FunctionType* function_type = llvm::FunctionType::get(builder.getInt64Ty(), false);
    llvm::Function* function = llvm::Function::Create(
        function_type,
        llvm::Function::ExternalLinkage,
        "jit_main",
        module
    );
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entry);

    // Build blocks
    while (true)
    {
        // Check for 16-bit alignment
        if ((cpu.pc & 0b1) != 0)
        {
            cpu.raise_exception(Exception::InstructionAddressMisaligned, cpu.pc);
            assert(false);
        }

        cpu.trace();

        // Fetch next instruction
        std::expected<Instruction, Exception> instruction =
            cpu.read_32(cpu.pc, CPU::AccessType::Instruction);
        assert(instruction.has_value());

        // Check it's valid
        if (instruction->instruction == 0xffffffff || instruction->instruction == 0)
        {
            cpu.raise_exception(Exception::IllegalInstruction, instruction->instruction);
            assert(false);
        }

        jit_context.current_instruction = instruction->instruction;
        assert(emit_instruction(cpu, jit_context));

        // Break if we encountered an instruction that requires "intervention"...
        if (jit_context.return_pc.has_value())
        {
            cpu.pc = *jit_context.return_pc;
            break;
        }

        // ...except JALR is special
        if (jit_context.emitted_jalr)
            break;

        cpu.pc = jit_context.pc + 4;
        jit_context.pc = cpu.pc;
    }

    // Return from block (unless JALR has done it for us)
    if (!jit_context.emitted_jalr)
        builder.CreateRet(llvm::ConstantInt::get(builder.getInt64Ty(), cpu.pc));

    // Build engine
    std::string error;
    llvm::ExecutionEngine* engine = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module))
        .setErrorStr(&error)
        .setOptLevel(llvm::CodeGenOptLevel::None)
        .setEngineKind(llvm::EngineKind::JIT)
        .create();

    if (!engine)
        throw std::runtime_error("failed to create llvm::ExecutionEngine: " + error);

    link_interface_functions(engine, jit_context);

    engine->finalizeObject();

#if DEBUG_JIT
    module->print(llvm::outs(), nullptr);
    assert(!llvm::verifyModule(*module, &llvm::errs()));
#endif

    // Run
    interface_cpu = &cpu;
#if DEBUG_JIT
    dbg("running frame");
#endif
    auto run = (u64(*)())engine->getFunctionAddress("jit_main");
    u64 next_pc = run();
    cpu.pc = next_pc;
#if DEBUG_JIT
    dbg("frame done");
#endif

    delete engine;
}

bool JIT::emit_instruction(CPU& cpu, Context& context)
{
    const u8 opcode = context.current_instruction.get_opcode();
    const u8 funct3 = context.current_instruction.get_funct3();
    const u8 funct7 = context.current_instruction.get_funct7();

    switch (opcode)
    {
        case OPCODES_BASE_R_TYPE:
        {
            if (funct7 == OPCODES_M_FUNCT_7)
                return false;

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
            if (funct7 == OPCODES_M_FUNCT_7)
                return false;

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

        default:
            return false;
    }

    return true;
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

void on_ecall(u64 pc)
{
    interface_cpu->pc = pc;
    ::ecall(*interface_cpu, Instruction(0));
}

void on_ebreak(u64 pc)
{
    interface_cpu->pc = pc;
    ::ebreak(*interface_cpu, Instruction(0));
}

void on_uret(u64 pc)
{
    interface_cpu->pc = pc;
    ::uret(*interface_cpu, Instruction(0));
}

void on_sret(u64 pc)
{
    interface_cpu->pc = pc;
    ::sret(*interface_cpu, Instruction(0));
}

void on_mret(u64 pc)
{
    interface_cpu->pc = pc;
    ::mret(*interface_cpu, Instruction(0));
}

void on_wfi(u64 pc)
{
    interface_cpu->pc = pc;
    ::mret(*interface_cpu, Instruction(0));
}

void on_sfence_vma(u64 pc)
{
    interface_cpu->pc = pc;
    ::mret(*interface_cpu, Instruction(0));
}

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

void on_csr(Instruction instruction, u64 pc)
{
    interface_cpu->pc = pc;
    ::opcodes_zicsr(*interface_cpu, instruction);
}

void on_atomic(Instruction instruction, u64 pc)
{
    interface_cpu->pc = pc;
    ::opcodes_a(*interface_cpu, instruction);
}

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

void JIT::register_interface_functions(
    llvm::Module* module,
    llvm::LLVMContext& context,
    Context& jit_context
)
{
    llvm::FunctionType* fallback_type = llvm::FunctionType::get
    (
        llvm::Type::getVoidTy(context),
        {
            llvm::Type::getInt32Ty(context),
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

    #define OPCODE(name, return_type)\
        jit_context.name = llvm::Function::Create(\
            return_type,\
            llvm::Function::ExternalLinkage,\
            TWINE_NAME(name),\
            module\
        )

    OPCODE(on_ecall,        OPCODE_TYPE_1(llvm::Type::getVoidTy(context)));
    OPCODE(on_ebreak,       OPCODE_TYPE_1(llvm::Type::getVoidTy(context)));
    OPCODE(on_uret,         OPCODE_TYPE_1(llvm::Type::getVoidTy(context)));
    OPCODE(on_sret,         OPCODE_TYPE_1(llvm::Type::getVoidTy(context)));
    OPCODE(on_mret,         OPCODE_TYPE_1(llvm::Type::getVoidTy(context)));
    OPCODE(on_wfi,          OPCODE_TYPE_1(llvm::Type::getVoidTy(context)));
    OPCODE(on_sfence_vma,   OPCODE_TYPE_1(llvm::Type::getVoidTy(context)));
    OPCODE(on_lb,           OPCODE_TYPE_2(llvm::Type::getInt8Ty(context)));
    OPCODE(on_lh,           OPCODE_TYPE_2(llvm::Type::getInt16Ty(context)));
    OPCODE(on_lw,           OPCODE_TYPE_2(llvm::Type::getInt32Ty(context)));
    OPCODE(on_ld,           OPCODE_TYPE_2(llvm::Type::getInt64Ty(context)));
    OPCODE(on_sb,           OPCODE_TYPE_3(llvm::Type::getInt8Ty(context)));
    OPCODE(on_sh,           OPCODE_TYPE_3(llvm::Type::getInt16Ty(context)));
    OPCODE(on_sw,           OPCODE_TYPE_3(llvm::Type::getInt32Ty(context)));
    OPCODE(on_sd,           OPCODE_TYPE_3(llvm::Type::getInt64Ty(context)));

    FALLBACK(on_csr);
    FALLBACK(on_atomic);
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

    LINK(on_csr);
    LINK(on_atomic);
}
