#include "opcodes_base.h"
#include <stdexcept>
#include <iostream>
#include "opcodes_zicsr.h"

// Helpers
u64 get_load_address(const CPU& cpu, const Instruction& instruction);
u64 get_store_address(const CPU& cpu, const Instruction& instruction);

bool opcodes_base(CPU& cpu, const Instruction& instruction)
{
    const u8 opcode = instruction.get_opcode();
    const u8 funct3 = instruction.get_funct3();
    const u8 funct7 = instruction.get_funct7();

    switch(opcode)
    {
        case OPCODES_BASE_R_TYPE:
        {
            switch(funct3)
            {
                case ADD:
                {
                    switch(funct7)
                    {
                        case 0:     add(cpu, instruction); break;
                        case SUB:   sub(cpu, instruction); break;
                        default:    return false;
                    }
                    break;
                }
                case OR:    _or (cpu, instruction); break;
                case AND:   _and(cpu, instruction); break;
                case SLTU:  sltu(cpu, instruction); break;
                default: return false;
            }
            break;
        }

        case OPCODES_BASE_I_TYPE:
        {
            switch(funct3)
            {
                case ADDI:  addi(cpu, instruction); break;
                case ORI:   ori (cpu, instruction); break;
                case ANDI:  andi(cpu, instruction); break;
                case SLLI:  slli(cpu, instruction); break;
                default:    return false;
            }
            break;
        }

        case OPCODES_BASE_LOAD_TYPE:
        {
            switch(funct3)
            {
                case LB:    lb(cpu, instruction); break;
                case LH:    lh(cpu, instruction); break;
                case LW:    lw(cpu, instruction); break;
                case LBU:   lbu(cpu, instruction); break;
                case LHU:   lhu(cpu, instruction); break;
            }
            break;
        }

        case OPCODES_BASE_S_TYPE:
        {
            switch(funct3)
            {
                case SB:    sb(cpu, instruction); break;
                case SH:    sh(cpu, instruction); break;
                case SW:    sw(cpu, instruction); break;
            }
            break;
        }

        case OPCODES_BASE_B_TYPE:
        {
            switch(funct3)
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
            // 0 = CSR instructions
            if (funct3 != 0) return false;

            const u8 rs2 = instruction.get_rs2();
            const u8 funct7 = instruction.get_funct7();

            if (rs2 == ECALL && funct7 == 0)
            {
                if (cpu.registers[10] == 0)
                    throw std::runtime_error("pass");
                else
                    throw std::runtime_error("ecall, x10 = " + std::to_string(cpu.registers[10]) + " (0 = pass)");
            }

            if (rs2 == EBREAK && funct7 == 0)
                throw std::runtime_error("ebreak");

            if (rs2 == URET && funct7 == 0)
                throw std::runtime_error("uret");

            if (rs2 == 2 && funct7 == SRET)
                throw std::runtime_error("sret");

            if (rs2 == 2 && funct7 == MRET)
            {
                mret(cpu, instruction);
                return true;
            }

            if (rs2 == 5 && funct7 == WFI)
                throw std::runtime_error("wfi");

            if (funct7 == SFENCE_VMA)  throw std::runtime_error("sfence.vma");
            if (funct7 == HFENCE_BVMA) throw std::runtime_error("hfence.bvma");
            if (funct7 == HFENCE_GVMA) throw std::runtime_error("hfence.gvma");

            return false;
        }

        case OPCODES_BASE_FENCE:
        {
            // No cores; no need!
            std::cout << "fence - ignoring" << std::endl;
            break;
        }

        case OPCODES_BASE_I_TYPE_32:
        {
            switch (funct3)
            {
                case ADDIW: addiw(cpu, instruction); break;
                default: return false;
            }
            break;
        }

        case OPCODES_BASE_R_TYPE_32:
        {
            switch (funct3)
            {
                case ADDW: addw(cpu, instruction); break;
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

void _or(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] = instruction.get_rs1() | instruction.get_rs2();
}

void _and(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] &
        cpu.registers[instruction.get_rs2()];
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

void orri(CPU& cpu, const Instruction& instruction)
{
    const i64 imm = instruction.get_imm(Instruction::Type::I);
    cpu.registers[instruction.get_rd()] = cpu.registers[instruction.get_rs1()] | imm;
}

void slli(CPU& cpu, const Instruction& instruction)
{
    const u8 shift_amount = instruction.get_shamt();
    cpu.registers[instruction.get_rd()] = cpu.registers[instruction.get_rs1()] << shift_amount;
}

void slti(CPU& cpu, const Instruction& instruction) {}
void sltiu(CPU& cpu, const Instruction& instruction) {}
void xori(CPU& cpu, const Instruction& instruction) {}
void sri(CPU& cpu, const Instruction& instruction) {}
void srli(CPU& cpu, const Instruction& instruction) {}
void srai(CPU& cpu, const Instruction& instruction) {}
void ori(CPU& cpu, const Instruction& instruction) {}

void andi(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] &
        instruction.get_imm(Instruction::Type::I);
}

void lb(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] = (u64)(i64)(i8)cpu.bus.read_8(
        get_load_address(cpu, instruction)
    );
}

void lh(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] = (u64)(i64)(i16)cpu.bus.read_16(
        get_load_address(cpu, instruction)
    );
}

void lw(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] = (u64)(i64)(i32)cpu.bus.read_32(
        get_load_address(cpu, instruction)
    );
}

void lbu(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] = cpu.bus.read_8(
        get_load_address(cpu, instruction)
    );
}

void lhu(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] = cpu.bus.read_16(
        get_load_address(cpu, instruction)
    );
}

void sb(CPU& cpu, const Instruction& instruction)
{
    cpu.bus.write_8(
        get_store_address(cpu, instruction),
        cpu.registers[instruction.get_rs2()]
    );
}

void sh(CPU& cpu, const Instruction& instruction)
{
    cpu.bus.write_16(
        get_store_address(cpu, instruction),
        cpu.registers[instruction.get_rs2()]
    );
}

void sw(CPU& cpu, const Instruction& instruction)
{
    cpu.bus.write_32(
        get_store_address(cpu, instruction),
        cpu.registers[instruction.get_rs2()]
    );
}

void beq(CPU& cpu, const Instruction& instruction)
{
    if (cpu.registers[instruction.get_rs1()] == cpu.registers[instruction.get_rs2()])
        cpu.pc += instruction.get_imm(Instruction::Type::B) - 4;
}

void bne(CPU& cpu, const Instruction& instruction)
{
    if (cpu.registers[instruction.get_rs1()] !=
        cpu.registers[instruction.get_rs2()])
        cpu.pc += instruction.get_imm(Instruction::Type::B) - 4;
}

void blt(CPU& cpu, const Instruction& instruction)
{
    if ((i64)cpu.registers[instruction.get_rs1()] <
        (i64)cpu.registers[instruction.get_rs2()])
        cpu.pc += instruction.get_imm(Instruction::Type::B) - 4;
}

void bge(CPU& cpu, const Instruction& instruction)
{
    if ((i64)cpu.registers[instruction.get_rs1()] >=
        (i64)cpu.registers[instruction.get_rs2()])
        cpu.pc += instruction.get_imm(Instruction::Type::B) - 4;
}

void bltu(CPU& cpu, const Instruction& instruction)
{
    if (cpu.registers[instruction.get_rs1()] <
        cpu.registers[instruction.get_rs2()])
        cpu.pc += instruction.get_imm(Instruction::Type::B) - 4;
}

void bgeu(CPU& cpu, const Instruction& instruction)
{
    if (cpu.registers[instruction.get_rs1()] >=
        cpu.registers[instruction.get_rs2()])
        cpu.pc += instruction.get_imm(Instruction::Type::B) - 4;
}

void jal(CPU& cpu, const Instruction& instruction)
{
    // Target register will contain pc + 4 (*not* for reason below though!)
    cpu.registers[instruction.get_rd()] = cpu.pc + 4;

    // Add offset to program counter - sign extension done for us
    const i64 offset = instruction.get_imm(Instruction::Type::J);
    cpu.pc += offset;

    // Minus 4 because 4 is always added anyway by caller
    cpu.pc -= 4;
}

void jalr(CPU& cpu, const Instruction& instruction)
{
    // Same as JAL but don't add (just set), and use register as well
    // as immediate value. Note the I-type encoding! The LSB is always
    // set to 0.
    cpu.registers[instruction.get_rd()] = cpu.pc + 4;
    i64 offset = instruction.get_imm(Instruction::Type::I);
    offset += cpu.registers[instruction.get_rs1()];
    offset &= 0xfffffffffffffffc;
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

void mret(CPU& cpu, const Instruction& instruction)
{
    /*
        "Returns from a machine-mode exception handler. Sets the pc to
        CSRs[mepc], the privilege mode to CSRs[mstatus].MPP,
        CSRs[mstatus].MIE to CSRs[mstatus].MPIE, and CSRs[mstatus].MPIE
        to 1; and, if user mode is supported, sets CSRs[mstatus].MPP to
        0".
     */

    // TODO: set status flags

    cpu.pc = read_csr(cpu, CSR_MEPEC).data - 4;
}

void addiw(CPU& cpu, const Instruction& instruction)
{
    const u64 result = cpu.registers[instruction.get_rs1()] +
        instruction.get_imm(Instruction::Type::I);

    // Take the lower 32 bits, then sign extend to 64
    const u64 extended = (i64)(i32)(result & 0xffffffff);
    cpu.registers[instruction.get_rd()] = extended;
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
