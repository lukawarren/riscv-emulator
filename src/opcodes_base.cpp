#include "opcodes_base.h"
#include <stdexcept>
#include <iostream>
#include "opcodes_zicsr.h"

bool opcodes_base(CPU& cpu, const Instruction& instruction)
{
    const u8 opcode = instruction.get_opcode();
    const u8 funct3 = instruction.get_funct3();

    switch(opcode)
    {
        case OPCODES_BASE_R_TYPE:
        {
            switch (funct3)
            {
                case OR: _or(cpu, instruction); break;
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
                case SLLI:  slli(cpu, instruction); break;
                default:    return false;
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
        case LUI:   lui  (cpu, instruction); break;
        case AUIPC: auipc(cpu, instruction); break;

        case OPCODES_BASE_SYSTEM:
        {
            // 0 = CSR instructions
            if (funct3 != 0) return false;

            const u8 rs2 = instruction.get_rs2();
            const u8 funct7 = instruction.get_funct7();

            if (rs2 == ECALL && funct7 == 0)
                throw std::runtime_error("ecall, x10 = " + std::to_string(cpu.registers[10]) + " (0 = pass)");

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

        default:
            return false;
    }

    return true;
}

void _or(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] = instruction.get_rs1() | instruction.get_rs2();
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
void andi(CPU& cpu, const Instruction& instruction) {}

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
    // Target register will contain pc + 4
    cpu.registers[instruction.get_rd()] = cpu.pc + 4;

    // Add offset to program counter - sign extension done for us
    i64 offset = instruction.get_imm(Instruction::Type::J);
    cpu.pc += offset;

    // Minus 4 because 4 is always added anyway by caller
    cpu.pc -= 4;
}

void lui(CPU& cpu, const Instruction& instruction)
{
    const u64 offset = instruction.get_imm(Instruction::Type::U) << 12;
    cpu.registers[instruction.get_rd()] = offset;
}

void auipc(CPU& cpu, const Instruction& instruction)
{
    const u64 offset = instruction.get_imm(Instruction::Type::U) << 12;
    cpu.pc += offset;
    cpu.registers[instruction.get_rd()] = cpu.pc;
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
    const u32 result = i32(
        cpu.registers[instruction.get_rs1()] +
        instruction.get_imm(Instruction::Type::I)
    );
    cpu.registers[instruction.get_rd()] = (u64)(i64)result;
}
