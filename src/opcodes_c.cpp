#include "opcodes_c.h"

bool opcodes_c(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 funct3 = instruction.get_funct3();
    const u8 opcode = instruction.get_opcode();

    switch (opcode)
    {
        case 0b00:
        {
            switch (funct3)
            {
                case C_LW:          c_lw(cpu, instruction); return true;
                case C_ADDI4SPN:    c_addi4spn(cpu, instruction); return true;
                default:            return false;
            }

            return false;
        }

        case 0b01:
        {
            switch (funct3)
            {
                case C_ADDI16SP:    c_addi16sp(cpu, instruction); return true;
                case C_NOP:         return true;
                default:            return false;
            }

            return false;
        }

        case 0b10:
        {
            switch (funct3)
            {
                case C_SLLI:        c_slli(cpu, instruction); return true;
                default: return     false;
            }

            return false;
        }

        default: return false;
    }

    return false;
}

void c_lw(CPU& cpu, const CompressedInstruction& instruction)
{
    // Equivalent to lw rd’, (4*imm) + (rs1’)
    std::cout << "addr = " << std::hex << instruction.get_rs1_alt() + instruction.get_imm(CompressedInstruction::Type::CL) * 4 << std::endl;
    std::optional<u32> value = cpu.bus.read_32(instruction.get_rs1_alt() + instruction.get_imm(CompressedInstruction::Type::CL) * 4);
    if (!value)
    {
        cpu.raise_exception(Exception::LoadAccessFault);
        return;
    }
    cpu.registers[instruction.get_rd_alt()] = (u64)(i64)(i32)*value;
}

void c_addi16sp(CPU& cpu, const CompressedInstruction& instruction)
{
    // Equivalent to addi sp, sp, 16*imm
    const u64 imm = instruction.get_none_zero_imm(CompressedInstruction::Type::CI);
    cpu.sp() = cpu.sp() + imm;
}

void c_addi4spn(CPU& cpu, const CompressedInstruction& instruction)
{
    // Equivalent to addi rd’, sp, 4*imm
    const u64 imm = instruction.get_none_zero_unsigned_imm(CompressedInstruction::Type::CIW);
    cpu.registers[instruction.get_rd_alt()] = cpu.sp() + imm;
}

void c_slli(CPU& cpu, const CompressedInstruction& instruction)
{
    // Equivalent to slli rd, rd, imm
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rd()] << instruction.get_shamt();
}
