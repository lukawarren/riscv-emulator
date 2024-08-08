#include "jit/jit.h"
#include "jit/jit_base.h"
#include "opcodes_base.h"
#include "opcodes_m.h"
#include "opcodes_zicsr.h"

using namespace JIT;

// "Interface" functions called from JIT'ed code itself
extern "C" void write_to_csr(u64 value, u64 address);

void JIT::init()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

void JIT::create_frame(CPU& cpu)
{
    // Init LLVM
    llvm::LLVMContext context;
    llvm::Module* module = new llvm::Module("jit", context);
    llvm::IRBuilder builder(context);

    // Create entry
    llvm::FunctionType* function_type = llvm::FunctionType::get(builder.getVoidTy(), false);
    llvm::Function* function = llvm::Function::Create(
        function_type,
        llvm::Function::ExternalLinkage,
        "main",
        module
    );
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entry);

    // Fetch variables we'll need
    llvm::Value* registers = get_registers(cpu, builder);
    Context jit_context {
        .builder = builder,
        .context = context,
        .registers = registers,
        .pc = cpu.pc,
        .write_to_csr = nullptr
    };

    register_interface_functions(module, context, jit_context);

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

        assert(emit_instruction(cpu, *instruction, jit_context));
        cpu.pc = jit_context.pc + 4;
        jit_context.pc = cpu.pc;
    }

    // Return from block
    builder.CreateRet(llvm::ConstantInt::get(builder.getInt64Ty(), cpu.pc));

    // Build engine
    std::string error;
    llvm::ExecutionEngine* engine = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module))
        .setErrorStr(&error)
        .setOptLevel(llvm::CodeGenOptLevel::Default)
        .create();

    if (!engine)
        throw std::runtime_error("failed to create llvm::ExecutionEngine: " + error);

    engine->finalizeObject();
    module->print(llvm::outs(), nullptr);

    // Run
    // void (*run)() = (void(*)())engine->getPointerToFunction(function);
    // run();

    delete engine;
}

bool JIT::emit_instruction(CPU& cpu, Instruction instruction, Context& context)
{
    const u8 opcode = instruction.get_opcode();
    const u8 funct3 = instruction.get_funct3();
    const u8 funct7 = instruction.get_funct7();

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
                        case 0:   add(instruction, context); break;
                        case SUB: sub(instruction, context); break;
                        default:  return false;
                    }
                    break;
                }
                case XOR: _xor(instruction, context); break;
                case OR:  _or (instruction, context); break;
                case AND: _and(instruction, context); break;
                case SLL:  sll(instruction, context); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7)
                    {
                        case SRL: srl(instruction, context); break;
                        case SRA: sra(instruction, context); break;
                        default:  return false;
                    }
                    break;
                }

                case SLT:  slt (instruction, context); break;
                case SLTU: sltu(instruction, context); break;
                default:   return false;
            }
            break;
        }

        case OPCODES_BASE_I_TYPE:
        {
            switch (funct3)
            {
                case ADDI: addi(instruction, context); break;
                case XORI: xori(instruction, context); break;
                case ORI:  ori (instruction, context); break;
                case ANDI: andi(instruction, context); break;
                case SLLI: slli(instruction, context); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7 & 0b11111110)
                    {
                        case SRAI: srai(instruction, context); break;
                        case 0:    srli(instruction, context); break;
                        default:   return false;
                    }
                    break;
                }

                case SLTI:  slti (instruction, context); break;
                case SLTIU: sltiu(instruction, context); break;
                default:    return false;
            }
            break;
        }

        case OPCODES_BASE_LOAD_TYPE:
        {
            switch (funct3)
            {
                case LB:    lb (instruction, context); break;
                case LH:    lh (instruction, context); break;
                case LW:    lw (instruction, context); break;
                case LBU:   lbu(instruction, context); break;
                case LHU:   lhu(instruction, context); break;

                // RV64I-specific
                case LWU:   lwu(instruction, context); break;
                case LD:    ld (instruction, context); break;
                default:    return false;
            }
            break;
        }

        case OPCODES_BASE_S_TYPE:
        {
            switch (funct3)
            {
                case SB:    sb(instruction, context); break;
                case SH:    sh(instruction, context); break;
                case SW:    sw(instruction, context); break;
                case SD:    sd(instruction, context); break; // RV64I
                default:    return false;
            }
            break;
        }

        case OPCODES_BASE_B_TYPE:
        {
            switch (funct3)
            {
                case BEQ:   beq (instruction, context); break;
                case BNE:   bne (instruction, context); break;
                case BLT:   blt (instruction, context); break;
                case BGE:   bge (instruction, context); break;
                case BLTU:  bltu(instruction, context); break;
                case BGEU:  bgeu(instruction, context); break;
                default:    return false;
            }
            break;
        }

        case JAL:   jal  (instruction, context); break;
        case JALR:  jalr (instruction, context); break;
        case LUI:   lui  (instruction, context); break;
        case AUIPC: auipc(instruction, context); break;

        case OPCODES_BASE_SYSTEM:
        {
            // Zicsr instructions
            if (funct3 != 0)
            {
                switch (funct3)
                {
                    case CSRRW:     csrrw (cpu, instruction); break;
                    case CSRRS:     csrrs (cpu, instruction); break;
                    case CSRRC:     csrrc (cpu, instruction); break;
                    case CSRRWI:    csrrwi(cpu, instruction); break;
                    case CSRRSI:    csrrsi(cpu, instruction); break;
                    case CSRRCI:    csrrci(cpu, instruction); break;

                    default:
                        return false;
                }
                break;
            }

            const u8 rs2 = instruction.get_rs2();

            if (rs2 == ECALL && funct7 == 0)
            {
                ecall(instruction, context);
                return true;
            }

            if (rs2 == EBREAK && funct7 == 0)
            {
                ebreak(instruction, context);
                return true;
            }

            if (rs2 == URET && funct7 == 0)
            {
                uret(instruction, context);
                return true;
            }

            if (rs2 == 2 && funct7 == SRET)
            {
                sret(instruction, context);
                return true;
            }

            if (rs2 == 2 && funct7 == MRET)
            {
                mret(instruction, context);
                return true;
            }

            if (rs2 == 5 && funct7 == WFI)
            {
                wfi(instruction, context);
                return true;
            }

            if (funct7 == SFENCE_VMA)
            {
                sfence_vma(instruction, context);
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
                case ADDIW: addiw(instruction, context); break;
                case SLLIW: slliw(instruction, context); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7)
                    {
                        case SRLIW: srliw(instruction, context); break;
                        case SRAIW: sraiw(instruction, context); break;
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
                        case ADDW:  addw(instruction, context); break;
                        case SUBW:  subw(instruction, context); break;
                        default:    return false;
                    }
                    break;
                }

                case SLLW: sllw(instruction, context); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7)
                    {
                        case SRLW:  srlw(instruction, context); break;
                        case SRAW:  sraw(instruction, context); break;
                        default:    return false;
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
    constexpr size_t len = sizeof(cpu.registers) / sizeof(cpu.registers[0]);
    llvm::Type* u64_type = builder.getInt64Ty();
    llvm::Type* registers_type = llvm::ArrayType::get(u64_type, len);
    llvm::PointerType* registers_pointer_type = registers_type->getPointerTo();

    return builder.CreateIntToPtr(
        llvm::ConstantInt::get(builder.getInt64Ty(), reinterpret_cast<uint64_t>(cpu.registers)),
        registers_pointer_type
    );
}

llvm::Value* JIT::load_register(Context& context, u32 index)
{
    llvm::Value* index_value = llvm::ConstantInt::get(context.builder.getInt32Ty(), index);
    return context.builder.CreateLoad(
        context.builder.getInt64Ty(),
        context.builder.CreateGEP(
            llvm::PointerType::get(context.builder.getInt64Ty(), 0),
            context.registers,
            index_value
        )
    );
}

void JIT::store_register(Context& context, u32 index, llvm::Value* value)
{
    // x0 is always zero
    if (index == 0)
        return;

    llvm::Value* index_value = llvm::ConstantInt::get(context.builder.getInt32Ty(), index);
    context.builder.CreateStore(
        value,
        context.builder.CreateGEP(
            llvm::PointerType::get(context.builder.getInt64Ty(), 0),
            context.registers,
            index_value
        )
    );
}

extern "C" void write_to_csr(u64 value, u64 address)
{
    dbg("todo: JIT csr write", value, address);
}

void JIT::register_interface_functions(
    llvm::Module* module,
    llvm::LLVMContext& context,
    Context& jit_context
)
{
    llvm::FunctionType* write_to_csr_type = llvm::FunctionType::get
    (
        llvm::Type::getVoidTy(context),         // Return
        {
            llvm::Type::getInt64Ty(context),    // Arg 1
            llvm::Type::getInt64Ty(context)     // Arg 2
        },
        false
    );

    jit_context.write_to_csr = llvm::Function::Create(
        write_to_csr_type,
        llvm::Function::ExternalLinkage,
        "",
        module
    );
}
