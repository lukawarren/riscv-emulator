#include "jit/jit.h"
#include "jit/jit_base.h"
#include "jit/jit_zicsr.h"
#include "opcodes_base.h"
#include "opcodes_m.h"
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

        // Break if we encountered an instruction that requires "intervention"
        if (jit_context.return_pc.has_value())
        {
            cpu.pc = *jit_context.return_pc;
            break;
        }

        cpu.pc = jit_context.pc + 4;
        jit_context.pc = cpu.pc;
    }

    // Return from block
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

void on_csr(Instruction instruction, u64 pc)
{
    interface_cpu->pc = pc;
    ::opcodes_zicsr(*interface_cpu, instruction);
}

void on_ecall(u64 pc)
{
    dbg("ecall");
    interface_cpu->pc = pc;
    ::ecall(*interface_cpu, Instruction(0));
}

void on_mret(u64 pc)
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

void print(u16 value)
{
    dbg(value);
    exit(0);
}

void JIT::register_interface_functions(
    llvm::Module* module,
    llvm::LLVMContext& context,
    Context& jit_context
)
{
    llvm::FunctionType* on_csr_type = llvm::FunctionType::get
    (
        llvm::Type::getVoidTy(context),
        {
            llvm::Type::getInt32Ty(context),
            llvm::Type::getInt64Ty(context)
        },
        false
    );

    jit_context.on_csr = llvm::Function::Create(
        on_csr_type,
        llvm::Function::ExternalLinkage,
        "on_csr",
        module
    );

    llvm::FunctionType* print_type = llvm::FunctionType::get
    (
        llvm::Type::getVoidTy(context),
        { llvm::Type::getInt16Ty(context) },
        false
    );

    jit_context.print = llvm::Function::Create(
        print_type,
        llvm::Function::ExternalLinkage,
        "print",
        module
    );

    #define OPCODE_TYPE_1(return_type) llvm::FunctionType::get\
    (\
        return_type,\
        { llvm::Type::getInt64Ty(context) },\
        false\
    )

    #define OPCODE_TYPE_2(return_type) llvm::FunctionType::get\
    (\
        return_type,\
        {\
            llvm::Type::getInt64Ty(context),\
            llvm::Type::getInt64Ty(context),\
            llvm::PointerType::get(llvm::Type::getInt1Ty(context), 0)\
        },\
        false\
    )

    #define TWINE_NAME(name) #name

    #define OPCODE(name, return_type) jit_context.name = llvm::Function::Create(\
        return_type,\
        llvm::Function::ExternalLinkage,\
        TWINE_NAME(name),\
        module\
    )

    OPCODE(on_ecall, OPCODE_TYPE_1(llvm::Type::getVoidTy(context)));
    OPCODE(on_mret,  OPCODE_TYPE_1(llvm::Type::getVoidTy(context)));
    OPCODE(on_lb,    OPCODE_TYPE_2(llvm::Type::getInt8Ty (context)));
    OPCODE(on_lh,    OPCODE_TYPE_2(llvm::Type::getInt16Ty(context)));
    OPCODE(on_lw,    OPCODE_TYPE_2(llvm::Type::getInt32Ty(context)));

    const llvm::DataLayout& dataLayout = module->getDataLayout();
    llvm::StructType* optionalStruct = llvm::StructType::create(
        context,
        { llvm::Type::getInt1Ty(context), llvm::Type::getInt16Ty(context) },
        "a", false
    );
    const llvm::StructLayout* structLayout = dataLayout.getStructLayout(optionalStruct);

    std::cout << "Size: " << structLayout->getSizeInBytes() << "\n";
    // std::cout << "Alignment: " << structLayout->getAlignment().Constant() << "\n";
    std::cout << "Offset of int1: " << structLayout->getElementOffset(0) << "\n";
    std::cout << "Offset of int16: " << structLayout->getElementOffset(1) << "\n";

    Optional<u16> bob = {};
    std::cout << (u64)((u64)&bob.has_value - (u64)&bob) << std::endl;
    std::cout << (u64)((u64)&bob.value - (u64)&bob) << std::endl;
    std::cout << sizeof(bob) << std::endl;
}

void JIT::link_interface_functions(
    llvm::ExecutionEngine* engine,
    Context& jit_context
)
{
    #define LINK(name)\
        engine->addGlobalMapping(jit_context.name, (void*)&name);

    LINK(print);
    LINK(on_csr);
    LINK(on_ecall);
    LINK(on_mret);
    LINK(on_lb);
    LINK(on_lh);
    LINK(on_lw);
}
