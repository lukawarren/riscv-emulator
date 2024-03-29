#include "opcodes_base.h"
#include <stdexcept>
#include "opcodes_zicsr.h"
#include "opcodes_m.h"

// Helpers
u64 get_load_address(const CPU& cpu, const Instruction& instruction);
u64 get_store_address(const CPU& cpu, const Instruction& instruction);
u32 get_wide_shift_amount(const CPU& cpu, const Instruction& instruction);
bool check_branch_alignment(CPU& cpu, const u64 target);

bool opcodes_base(CPU& cpu, const Instruction& instruction)
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
                        case 0:     add(cpu, instruction); break;
                        case SUB:   sub(cpu, instruction); break;
                        default:    return false;
                    }
                    break;
                }
                case XOR:   _xor (cpu, instruction); break;
                case OR:    _or  (cpu, instruction); break;
                case AND:   _and (cpu, instruction); break;
                case SLL:   sll  (cpu, instruction); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7)
                    {
                        case SRL: srl(cpu, instruction); break;
                        case SRA: sra(cpu, instruction); break;
                        default:  return false;
                    }
                    break;
                }

                case SLT:   slt (cpu, instruction); break;
                case SLTU:  sltu(cpu, instruction); break;
                default: return false;
            }
            break;
        }

        case OPCODES_BASE_I_TYPE:
        {
            switch (funct3)
            {
                case ADDI:  addi(cpu, instruction); break;
                case XORI:  xori(cpu, instruction); break;
                case ORI:   ori (cpu, instruction); break;
                case ANDI:  andi(cpu, instruction); break;
                case SLLI:  slli(cpu, instruction); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7 & 0b11111110)
                    {
                        case SRAI:  srai(cpu, instruction); break;
                        case 0:     srli(cpu, instruction); break;
                        default:    return false;
                    }
                    break;
                }

                case SLTI:   slti (cpu, instruction); break;
                case SLTIU:  sltiu(cpu, instruction); break;

                default: return false;
            }
            break;
        }

        case OPCODES_BASE_LOAD_TYPE:
        {
            switch (funct3)
            {
                case LB:    lb (cpu, instruction); break;
                case LH:    lh (cpu, instruction); break;
                case LW:    lw (cpu, instruction); break;
                case LBU:   lbu(cpu, instruction); break;
                case LHU:   lhu(cpu, instruction); break;

                // RV64I-specific
                case LWU:   lwu(cpu, instruction); break;
                case LD:    ld (cpu, instruction); break;
                default:    return false;
            }
            break;
        }

        case OPCODES_BASE_S_TYPE:
        {
            switch (funct3)
            {
                case SB:    sb(cpu, instruction); break;
                case SH:    sh(cpu, instruction); break;
                case SW:    sw(cpu, instruction); break;
                case SD:    sd(cpu, instruction); break; // RV64I
                default:    return false;
            }
            break;
        }

        case OPCODES_BASE_B_TYPE:
        {
            switch (funct3)
            {
                case BEQ:   beq (cpu, instruction); break;
                case BNE:   bne (cpu, instruction); break;
                case BLT:   blt (cpu, instruction); break;
                case BGE:   bge (cpu, instruction); break;
                case BLTU:  bltu(cpu, instruction); break;
                case BGEU:  bgeu(cpu, instruction); break;
                default:    return false;
            }
            break;
        }

        case JAL:   jal  (cpu, instruction); break;
        case JALR:  jalr (cpu, instruction); break;

        case LUI:   lui  (cpu, instruction); break;
        case AUIPC: auipc(cpu, instruction); break;

        case OPCODES_BASE_SYSTEM:
        {
            // 0 = CSR instructions - except for ebreak
            if (funct3 != 0) return false;

            const u8 rs2 = instruction.get_rs2();

            if (rs2 == ECALL && funct7 == 0)
            {
                ecall(cpu, instruction);
                return true;
            }

            if (rs2 == EBREAK && funct7 == 0)
            {
                ebreak(cpu, instruction);
                return true;
            }

            if (rs2 == URET && funct7 == 0)
            {
                uret(cpu, instruction);
                return true;
            }

            if (rs2 == 2 && funct7 == SRET)
            {
                sret(cpu, instruction);
                return true;
            }

            if (rs2 == 2 && funct7 == MRET)
            {
                mret(cpu, instruction);
                return true;
            }

            if (rs2 == 5 && funct7 == WFI)
            {
                wfi(cpu, instruction);
                return true;
            }

            if (funct7 == SFENCE_VMA)
            {
                sfence_vma(cpu, instruction);
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
                case ADDIW: addiw(cpu, instruction); break;
                case SLLIW: slliw(cpu, instruction); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7)
                    {
                        case SRLIW: srliw(cpu, instruction); break;
                        case SRAIW: sraiw(cpu, instruction); break;
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
                        case ADDW:  addw(cpu, instruction); break;
                        case SUBW:  subw(cpu, instruction); break;
                        default:    return false;
                    }
                    break;
                }

                case SLLW: sllw(cpu, instruction); break;

                case OPCODES_SHIFT_RIGHT:
                {
                    switch (funct7)
                    {
                        case SRLW:  srlw(cpu, instruction); break;
                        case SRAW:  sraw(cpu, instruction); break;
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

void add(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] +
        cpu.registers[instruction.get_rs2()];
}

void sub(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] -
        cpu.registers[instruction.get_rs2()];
}

void _xor(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] ^
        cpu.registers[instruction.get_rs2()];
}

void _or(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] |
        cpu.registers[instruction.get_rs2()];
}

void _and(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] &
        cpu.registers[instruction.get_rs2()];
}

void sll(CPU& cpu, const Instruction& instruction)
{
    // For RV64I, 6 bits are used (RV32I = 5)!,
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] <<
        cpu.registers[instruction.get_rs2_6_bits()];
}

void srl(CPU& cpu, const Instruction& instruction)
{
    // For RV64I, 6 bits are used (RV32I = 5)!,
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] >>
        cpu.registers[instruction.get_rs2_6_bits()];
}

void sra(CPU& cpu, const Instruction& instruction)
{
    // For RV64I, 6 bits are used (RV32I = 5)!,
    cpu.registers[instruction.get_rd()] =
        i64(cpu.registers[instruction.get_rs1()]) >>
        cpu.registers[instruction.get_rs2_6_bits()];
}

void slt(CPU& cpu, const Instruction& instruction)
{
    // Signed comparison between rs1 and rs2
    const i64 rs1 = cpu.registers[instruction.get_rs1()];
    const i64 rs2 = cpu.registers[instruction.get_rs2()];
    cpu.registers[instruction.get_rd()] = (rs1 < rs2) ? 1 : 0;
}

void sltu(CPU& cpu, const Instruction& instruction)
{
    // Unsigned comparison between rs1 and rs2 (i.e. zero extends)
    const u64 rs1 = cpu.registers[instruction.get_rs1()];
    const u64 rs2 = cpu.registers[instruction.get_rs2()];
    cpu.registers[instruction.get_rd()] = (rs1 < rs2) ? 1 : 0;
}

void addi(CPU& cpu, const Instruction& instruction)
{
    // No need for overflow checks
    const i64 imm = instruction.get_imm(Instruction::Type::I);
    cpu.registers[instruction.get_rd()] = cpu.registers[instruction.get_rs1()] + imm;
}

void xori(CPU& cpu, const Instruction& instruction)
{
    const i64 imm = instruction.get_imm(Instruction::Type::I);
    cpu.registers[instruction.get_rd()] = cpu.registers[instruction.get_rs1()] ^ imm;
}

void ori(CPU& cpu, const Instruction& instruction)
{
    const i64 imm = instruction.get_imm(Instruction::Type::I);
    cpu.registers[instruction.get_rd()] = cpu.registers[instruction.get_rs1()] | imm;
}

void andi(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] &
        instruction.get_imm(Instruction::Type::I);
}

void slli(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] << instruction.get_shamt();
}

void srli(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] >> instruction.get_shamt();
}

void srai(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        (i64)(cpu.registers[instruction.get_rs1()]) >> instruction.get_shamt();
}

void slti(CPU& cpu, const Instruction& instruction)
{
    // Signed comparison between rs1 and imm
    const i64 rs1 = cpu.registers[instruction.get_rs1()];
    const i64 rs2 = instruction.get_imm(Instruction::Type::I);
    cpu.registers[instruction.get_rd()] = (rs1 < rs2) ? 1 : 0;
}

void sltiu(CPU& cpu, const Instruction& instruction)
{
    // Unsigned comparison between rs1 and imm
    const u64 rs1 = cpu.registers[instruction.get_rs1()];
    const u64 rs2 = instruction.get_imm(Instruction::Type::I);
    cpu.registers[instruction.get_rd()] = (rs1 < rs2) ? 1 : 0;
}

void lb(CPU& cpu, const Instruction& instruction)
{
    std::optional<u8> value = cpu.bus.read_8(get_load_address(cpu, instruction));
    if (!value)
    {
        cpu.raise_exception(Exception::LoadAccessFault);
        return;
    }
    cpu.registers[instruction.get_rd()] = (u64)(i64)(i8)*value;
}

void lh(CPU& cpu, const Instruction& instruction)
{
    std::optional<u16> value = cpu.bus.read_16(get_load_address(cpu, instruction));
    if (!value)
    {
        cpu.raise_exception(Exception::LoadAccessFault);
        return;
    }
    cpu.registers[instruction.get_rd()] = (u64)(i64)(i16)*value;
}

void lw(CPU& cpu, const Instruction& instruction)
{
    std::optional<u32> value = cpu.bus.read_32(get_load_address(cpu, instruction));
    if (!value)
    {
        cpu.raise_exception(Exception::LoadAccessFault);
        return;
    }
    cpu.registers[instruction.get_rd()] = (u64)(i64)(i32)*value;
}

void lbu(CPU& cpu, const Instruction& instruction)
{
    std::optional<u8> value = cpu.bus.read_8(get_load_address(cpu, instruction));
    if (!value)
    {
        cpu.raise_exception(Exception::LoadAccessFault);
        return;
    }
    cpu.registers[instruction.get_rd()] = *value;
}

void lhu(CPU& cpu, const Instruction& instruction)
{
    std::optional<u16> value = cpu.bus.read_16(get_load_address(cpu, instruction));
    if (!value)
    {
        cpu.raise_exception(Exception::LoadAccessFault);
        return;
    }
    cpu.registers[instruction.get_rd()] = *value;
}

void sb(CPU& cpu, const Instruction& instruction)
{
    if (!cpu.bus.write_8(
        get_store_address(cpu, instruction),
        cpu.registers[instruction.get_rs2()]
    ))
        cpu.raise_exception(Exception::StoreOrAMOAccessFault);
}

void sh(CPU& cpu, const Instruction& instruction)
{
    if (!cpu.bus.write_16(
        get_store_address(cpu, instruction),
        cpu.registers[instruction.get_rs2()]
    ))
        cpu.raise_exception(Exception::StoreOrAMOAccessFault);
}

void sw(CPU& cpu, const Instruction& instruction)
{
    if (!cpu.bus.write_32(
        get_store_address(cpu, instruction),
        cpu.registers[instruction.get_rs2()]
    ))
        cpu.raise_exception(Exception::StoreOrAMOAccessFault);
}

void beq(CPU& cpu, const Instruction& instruction)
{
    const u64 target = instruction.get_imm(Instruction::Type::B);

    if (cpu.registers[instruction.get_rs1()] == cpu.registers[instruction.get_rs2()] &&
        check_branch_alignment(cpu, target))
        cpu.pc += target - 4;
}

void bne(CPU& cpu, const Instruction& instruction)
{
    const u64 target = instruction.get_imm(Instruction::Type::B);

    if (cpu.registers[instruction.get_rs1()] !=
        cpu.registers[instruction.get_rs2()] &&
        check_branch_alignment(cpu, target))
        cpu.pc += target - 4;
}

void blt(CPU& cpu, const Instruction& instruction)
{
    const u64 target = instruction.get_imm(Instruction::Type::B);

    if ((i64)cpu.registers[instruction.get_rs1()] <
        (i64)cpu.registers[instruction.get_rs2()] &&
        check_branch_alignment(cpu, target))
        cpu.pc += target - 4;
}

void bge(CPU& cpu, const Instruction& instruction)
{
    const u64 target = instruction.get_imm(Instruction::Type::B);

    if ((i64)cpu.registers[instruction.get_rs1()] >=
        (i64)cpu.registers[instruction.get_rs2()] &&
        check_branch_alignment(cpu, target))
        cpu.pc += target - 4;
}

void bltu(CPU& cpu, const Instruction& instruction)
{
    const u64 target = instruction.get_imm(Instruction::Type::B);

    if (cpu.registers[instruction.get_rs1()] <
        cpu.registers[instruction.get_rs2()] &&
        check_branch_alignment(cpu, target))
        cpu.pc += target - 4;
}

void bgeu(CPU& cpu, const Instruction& instruction)
{
    const u64 target = instruction.get_imm(Instruction::Type::B);

    if (cpu.registers[instruction.get_rs1()] >=
        cpu.registers[instruction.get_rs2()] &&
        check_branch_alignment(cpu, target))
        cpu.pc += target - 4;
}

void jal(CPU& cpu, const Instruction& instruction)
{
    // Add offset to program counter - sign extension done for us
    const i64 offset = instruction.get_imm(Instruction::Type::J);

    // Alignment check (see jalr)
    if (!check_branch_alignment(cpu, offset))
        return;

    // Target register will contain pc + 4 (*not* for reason below though!)
    cpu.registers[instruction.get_rd()] = cpu.pc + 4;

    // Minus 4 because 4 is always added anyway by caller
    cpu.pc += offset - 4;
}

void jalr(CPU& cpu, const Instruction& instruction)
{
    // Same as JAL but don't add (just set), and use register as well
    // as immediate value. Note the I-type encoding! The LSB is always
    // set to 0.

    i64 offset = instruction.get_imm(Instruction::Type::I);
    offset += cpu.registers[instruction.get_rs1()];
    offset &= 0xfffffffffffffffe;

    // An instruction address misaligned exception is generated on a
    // taken branch or unconditional jump if the target address is not
    // four-byte aligned
    if (!check_branch_alignment(cpu, offset))
        return;

    cpu.registers[instruction.get_rd()] = cpu.pc + 4;
    cpu.pc = (u64)offset - 4;
}

void lui(CPU& cpu, const Instruction& instruction)
{
    const u64 offset = instruction.get_imm(Instruction::Type::U);
    cpu.registers[instruction.get_rd()] = offset;
}

void auipc(CPU& cpu, const Instruction& instruction)
{
    const u64 offset = instruction.get_imm(Instruction::Type::U);
    cpu.registers[instruction.get_rd()] = cpu.pc + offset;
}

void ecall(CPU& cpu, const Instruction& instruction)
{
    if (cpu.emulating_test)
    {
        /*
            RISC-V tests use an ecall to signal the test is over.
            A 0 in x10 represents a pass.
         */
        if (cpu.registers[10] == 0) throw std::string("pass");
        else throw std::string("fail");
    }

    // Normal operation
    switch (cpu.privilege_level)
    {
        case PrivilegeLevel::User:
            cpu.raise_exception(Exception::EnvironmentCallFromUMode);
            break;

        case PrivilegeLevel::Supervisor:
            cpu.raise_exception(Exception::EnvironmentCallFromSMode);
            break;

        case PrivilegeLevel::Machine:
            throw std::runtime_error("TODO: unsupported ecall from machine mode - possible SBI driver");
            cpu.raise_exception(Exception::EnvironmentCallFromMMode);
            break;

        default:
            cpu.raise_exception(Exception::IllegalInstruction);
            break;
    }
}

void ebreak(CPU& cpu, const Instruction& instruction)
{
    // Used for syscalls, etc.
    cpu.raise_exception(Exception::Breakpoint, 0);
}

void uret(CPU& cpu, const Instruction& instruction)
{
    throw std::runtime_error("uret unsupported");
}

void sret(CPU& cpu, const Instruction& instruction)
{
    // When TSR=1, attempts to execute SRET while executing in S-mode
    // will raise an illegal instruction exception.
    if (cpu.mstatus.fields.tsr == 1)
    {
        cpu.raise_exception(Exception::IllegalInstruction);
        return;
    }

    // Otherwise virtually the same as mret (see below)

    if (cpu.privilege_level < PrivilegeLevel::Supervisor)
    {
        cpu.raise_exception(Exception::IllegalInstruction);
        return;
    }

    if (cpu.mstatus.fields.spp != (u64)PrivilegeLevel::Machine)
        cpu.mstatus.fields.mprv = 0;

    cpu.pc = *read_csr(cpu, CSR_SEPC) - 4;
    cpu.privilege_level = (PrivilegeLevel)cpu.mstatus.fields.spp;
    cpu.mstatus.fields.sie = cpu.mstatus.fields.spie;
    cpu.mstatus.fields.spie = 1;
    cpu.mstatus.fields.spp = 0;
}

void mret(CPU& cpu, const Instruction& instruction)
{
    /*
        "Returns from a machine-mode exception handler. Sets the pc to
        CSRs[mepc], the privilege mode to CSRs[mstatus].MPP,
        CSRs[mstatus].MIE to CSRs[mstatus].MPIE, and CSRs[mstatus].MPIE
        to 1; and, if user mode is supported, sets CSRs[mstatus].MPP to
        0".
     */

    // Must be in machine mode or higher
    if (cpu.privilege_level < PrivilegeLevel::Machine)
    {
        cpu.raise_exception(Exception::IllegalInstruction);
        return;
    }

    if (cpu.mstatus.fields.mpp != (u64)PrivilegeLevel::Machine)
        cpu.mstatus.fields.mprv = 0;

    cpu.pc = *cpu.mepc.read(cpu) - 4;
    cpu.privilege_level = (PrivilegeLevel)cpu.mstatus.fields.mpp;
    cpu.mstatus.fields.mie = cpu.mstatus.fields.mpie;
    cpu.mstatus.fields.mpie = 1;
    cpu.mstatus.fields.mpp = 0;
}

void wfi(CPU& cpu, const Instruction& instruction)
{
    /*
        The Wait for Interrupt instruction (WFI) provides a hint to the
        implementation that the current hart can be stalled until an interrupt
        might need servicing.

        The purpose of the WFI instruction is to provide a hint to the
        implementation, and so a legal implementation is to simply implement
        WFI as a NOP.

        This instruction may raise an illegal instruction exception when TW=1
        in mstatus.

        When S-mode is implemented, then executing WFI in U-mode causes an
        illegal instruction exception, unless it completes within an
        implementation-specific, bounded time limit.
     */

    if (cpu.mstatus.fields.tw == 1 || cpu.privilege_level == PrivilegeLevel::User)
    {
        cpu.raise_exception(Exception::IllegalInstruction);
        return;
    }
}

void sfence_vma(CPU& cpu, const Instruction& instruction)
{
    // Synchronises updates to in-memory memory-management data structres with
    // current execution. i.e. glorified NOP for us.
    // Have to trap when TVM = 1.
    if (cpu.mstatus.fields.tvm == 1)
        cpu.raise_exception(Exception::IllegalInstruction);
}

void lwu(CPU& cpu, const Instruction& instruction)
{
    std::optional<u32> value = cpu.bus.read_32(get_load_address(cpu, instruction));
    if (!value)
    {
        cpu.raise_exception(Exception::LoadAccessFault);
        return;
    }
    cpu.registers[instruction.get_rd()] = *value;
}

void ld(CPU& cpu, const Instruction& instruction)
{
    std::optional<u64> value = cpu.bus.read_64(get_load_address(cpu, instruction));
    if (!value)
    {
        cpu.raise_exception(Exception::LoadAccessFault);
        return;
    }
    cpu.registers[instruction.get_rd()] = (u64)(i64)*value;
}

void sd(CPU& cpu, const Instruction& instruction)
{
    if (!cpu.bus.write_64(
        get_store_address(cpu, instruction),
        cpu.registers[instruction.get_rs2()]
    ))
        cpu.raise_exception(Exception::StoreOrAMOAccessFault);
}

void addiw(CPU& cpu, const Instruction& instruction)
{
    const u64 result = cpu.registers[instruction.get_rs1()] +
        instruction.get_imm(Instruction::Type::I);

    // Take the lower 32 bits, then sign extend to 64
    const u64 extended = (i64)(i32)(result & 0xffffffff);
    cpu.registers[instruction.get_rd()] = extended;
}

void slliw(CPU& cpu, const Instruction& instruction)
{
    const u32 shift_amount = get_wide_shift_amount(cpu, instruction);
    cpu.registers[instruction.get_rd()] =
        (i64)(i32)(cpu.registers[instruction.get_rs1()] << shift_amount);
}

void srliw(CPU& cpu, const Instruction& instruction)
{
    const u32 shift_amount = get_wide_shift_amount(cpu, instruction);
    const u32 rs1 = (u32)cpu.registers[instruction.get_rs1()];
    cpu.registers[instruction.get_rd()] = (i64)(i32)(rs1 >> shift_amount);
}

void sraiw(CPU& cpu, const Instruction& instruction)
{
    const u32 shift_amount = get_wide_shift_amount(cpu, instruction);
    const i32 rs1 = (i32)cpu.registers[instruction.get_rs1()];
    cpu.registers[instruction.get_rd()] = (u64)(i64)(rs1 >> shift_amount);
}

void addw(CPU& cpu, const Instruction& instruction)
{
    const u64 result =
        cpu.registers[instruction.get_rs1()] +
        cpu.registers[instruction.get_rs2()];

    // Take the lower 32 bits, then sign extend to 64
    const u64 extended = (i64)(i32)(result & 0xffffffff);
    cpu.registers[instruction.get_rd()] = extended;
}

void subw(CPU& cpu, const Instruction& instruction)
{
    const u64 result =
        cpu.registers[instruction.get_rs1()] -
        cpu.registers[instruction.get_rs2()];

    // Take the lower 32 bits, then sign extend to 64
    const u64 extended = (i64)(i32)(result & 0xffffffff);
    cpu.registers[instruction.get_rd()] = extended;
}

void sllw(CPU& cpu, const Instruction& instruction)
{
    const u8 shift_amount = cpu.registers[instruction.get_rs2()] & 0b11111;
    cpu.registers[instruction.get_rd()] =
        (i64)(i32)(cpu.registers[instruction.get_rs1()] << shift_amount);
}

void srlw(CPU& cpu, const Instruction& instruction)
{
    const u8 shift_amount = cpu.registers[instruction.get_rs2()] & 0b11111;
    const u32 rs1 = (u32)cpu.registers[instruction.get_rs1()];
    cpu.registers[instruction.get_rd()] = (i64)(i32)(rs1 >> shift_amount);
}

void sraw(CPU& cpu, const Instruction& instruction)
{
    const u8 shift_amount = cpu.registers[instruction.get_rs2()] & 0b11111;
    const i32 rs1 = (i32)cpu.registers[instruction.get_rs1()];
    cpu.registers[instruction.get_rd()] = (i64)(rs1 >> shift_amount);
}

// -- Helpers --

u64 get_load_address(const CPU& cpu, const Instruction& instruction)
{
    return instruction.get_imm(Instruction::Type::I) +
        cpu.registers[instruction.get_rs1()];
}

u64 get_store_address(const CPU& cpu, const Instruction& instruction)
{
    return instruction.get_imm(Instruction::Type::S) +
        cpu.registers[instruction.get_rs1()];
}

u32 get_wide_shift_amount(const CPU&, const Instruction& instruction)
{
    // SLLIW, SRLIW and SRAIW generate an illegal instruction exception
    // if imm[5] != 0. TODO: emulate!
    return instruction.get_imm(Instruction::Type::I) & 0b11111;
}

bool check_branch_alignment(CPU& cpu, const u64 target)
{
    // Needs to be 16-bit aligned (would be 32 if we didn't support RVC)
    if ((target & 0b1) != 0)
    {
        cpu.raise_exception(Exception::InstructionAddressMisaligned, 0);
        return false;
    }

    return true;
}
