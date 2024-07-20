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
                case C_LW:          c_lw(cpu, instruction);         return true;
                case C_LD:          c_ld(cpu, instruction);         return true;
                case C_SW:          c_sw(cpu, instruction);         return true;
                case C_SD:          c_sd(cpu, instruction);         return true;
                case C_ADDI4SPN:    c_addi4spn(cpu, instruction);   return true;
                default:                                            return false;
            }

            return false;
        }

        case 0b01:
        {
            switch (funct3)
            {
                case C_LI:          c_li(cpu, instruction);         return true;
                case C_J:           c_j(cpu, instruction);          return true;
                case C_BEQZ:        c_beqz(cpu, instruction);       return true;
                case C_BNEZ:        c_bnez(cpu, instruction);       return true;
                case C_ADDI:        c_addi(cpu, instruction);       return true;
                case C_ADDIW:       c_addiw(cpu, instruction);      return true;
                case C_ADDI16SP:
                {
                    switch (instruction.get_rd())
                    {
                        case 0:                                     return true; // NOP
                        case 2:     c_addi16sp(cpu, instruction);   return true;
                        default:    c_lui(cpu, instruction);        return true;
                    }
                }
                case 0b100:
                {
                    switch (instruction.get_funct2())
                    {
                        case C_SRLI: c_srli(cpu, instruction);      return true;
                        case C_SRAI: c_srai(cpu, instruction);      return true;
                        case C_ANDI: c_andi(cpu, instruction);      return true;
                        case 0b11:
                        {
                            const u8 a = (instruction.instruction >> 12) & 0b1;
                            const u8 b = (instruction.instruction >> 5) & 0b11;

                            if (a == 0 && b == 0)
                            {
                                c_sub(cpu, instruction);
                                return true;
                            }

                            if (a == 0 && b == 1)
                            {
                                c_xor(cpu, instruction);
                                return true;
                            }

                            if (a == 0 && b == 2)
                            {
                                c_or(cpu, instruction);
                                return true;
                            }

                            if (a == 0 && b == 3)
                            {
                                c_and(cpu, instruction);
                                return true;
                            }

                            if (a == 1 && b == 0)
                            {
                                c_subw(cpu, instruction);
                                return true;
                            }

                            if (a == 1 && b == 1)
                            {
                                c_addw(cpu, instruction);
                                return true;
                            }

                            return false;
                        }
                        default:                                    return false;
                    }
                }
                default:                                            return false;
            }

            return false;
        }

        case 0b10:
        {
            switch (funct3)
            {
                case C_LWSP:        c_lwsp(cpu, instruction);       return true;
                case C_LDSP:        c_ldsp(cpu, instruction);       return true;
                case C_SLLI:        c_slli(cpu, instruction);       return true;

                case 0b100:
                {
                    const u8 a = (instruction.instruction >> 12) & 0b1;
                    const u8 b = (instruction.instruction >> 2) & 0x1f;

                    if (a == 0 && b == 0)
                    {
                        c_jr(cpu, instruction);
                        return true;
                    }

                    else if (a == 0)
                    {
                        c_mv(cpu, instruction);
                        return true;
                    }

                    if (a == 1 && b == 0)
                    {
                        if (instruction.get_rd() != 0)
                        {
                            c_jalr(cpu, instruction);
                            return true;
                        }
                        else
                        {
                            c_ebreak(cpu, instruction);
                            return true;
                        }

                        return false;
                    }

                    if (a == 1)
                    {
                        c_add(cpu, instruction);
                        return true;
                    }

                    return false;
                }

                case C_SWSP:        c_swsp(cpu, instruction);       return true;
                case C_SDSP:        c_sdsp(cpu, instruction);       return true;

                default:                                            return false;
            }

            return false;
        }

        default: return false;
    }

    return false;
}

void c_lw(CPU& cpu, const CompressedInstruction& instruction)
{
    const auto value = cpu.read_32(
        cpu.registers[instruction.get_rs1_alt()] +
        instruction.get_imm(CompressedInstruction::Type::CL)
    );
    if (!value)
    {
        cpu.raise_exception(value.error());
        return;
    }
    cpu.registers[instruction.get_rd_alt()] = (u64)(i64)(i32)*value;
}

void c_ld(CPU& cpu, const CompressedInstruction& instruction)
{
    const auto value = cpu.read_64(
        cpu.registers[instruction.get_rs1_alt()] + instruction.get_ld_sd_imm()
    );
    if (!value)
    {
        cpu.raise_exception(value.error());
        return;
    }
    cpu.registers[instruction.get_rd_alt()] = *value;
}

void c_lwsp(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd();
    const u64 offset = instruction.get_lwsp_offset();
    const auto value = cpu.read_32(cpu.registers[2] + offset);

    if (!value)
    {
        cpu.raise_exception(value.error());
        return;
    }

    cpu.registers[rd] = (u64)(i64)(i32)*value;
}

void c_ldsp(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd();
    const u64 offset = instruction.get_ldsp_offset();
    const auto value = cpu.read_64(cpu.registers[2] + offset);

    if (!value)
    {
        cpu.raise_exception(value.error());
        return;
    }

    cpu.registers[rd] = *value;
}

void c_sw(CPU& cpu, const CompressedInstruction& instruction)
{
    const auto error = cpu.write_32(
        cpu.registers[instruction.get_rs1_alt()] +
        instruction.get_imm(CompressedInstruction::Type::CL),
        cpu.registers[instruction.get_rs2_alt()]
    );
    if (error.has_value())
    {
        cpu.raise_exception(*error);
        return;
    }
}

void c_sd(CPU& cpu, const CompressedInstruction& instruction)
{
    const auto error = cpu.write_64(
        cpu.registers[instruction.get_rs1_alt()] +
        instruction.get_ld_sd_imm(),
        cpu.registers[instruction.get_rs2_alt()]
    );
    if (error.has_value())
    {
        cpu.raise_exception(*error);
        return;
    }
}

void c_swsp(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rs2 = instruction.get_rs2();
    const u64 offset = instruction.get_swsp_offset();
    const auto error = cpu.write_32(
        cpu.registers[2] + offset,
        cpu.registers[rs2]
    );

    if (error.has_value())
    {
        cpu.raise_exception(*error);
        return;
    }
}

void c_sdsp(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rs2 = instruction.get_rs2();
    const u64 offset = instruction.get_sdsp_offset();
    const auto error = cpu.write_64(
        cpu.registers[2] + offset,
        cpu.registers[rs2]
    );

    if (error.has_value())
    {
        cpu.raise_exception(*error);
        return;
    }
}

void c_j(CPU& cpu, const CompressedInstruction& instruction)
{
    // Minus 2 as will be added by the caller
    cpu.pc += instruction.get_jump_offset() - 2;
}

void c_jr(CPU& cpu, const CompressedInstruction& instruction)
{
    // Minus 2 as will be added by the caller
    const u8 rs1 = instruction.get_rs1();
    if (rs1 != 0)
        cpu.pc = cpu.registers[rs1] - 2;
}

void c_jalr(CPU& cpu, const CompressedInstruction& instruction)
{
    // Minus 2 as will be added by the caller
    const u8 rs1 = instruction.get_rs1();
    const u64 t = cpu.pc + 2;
    cpu.pc = cpu.registers[rs1] - 2;
    cpu.registers[1] = t;
}

void c_beqz(CPU& cpu, const CompressedInstruction& instruction)
{
    // Minus 2 as will be added by the caller
    const u8 rs1 = instruction.get_rd_with_offset();
    if (cpu.registers[rs1] == 0)
        cpu.pc += instruction.get_branch_offset() - 2;
}

void c_bnez(CPU& cpu, const CompressedInstruction& instruction)
{
    // Minus 2 as will be added by the caller
    const u8 rs1 = instruction.get_rd_with_offset();
    if (cpu.registers[rs1] != 0)
        cpu.pc += instruction.get_branch_offset() - 2;
}

void c_addi(CPU& cpu, const CompressedInstruction& instruction)
{
    const u64 imm = instruction.get_none_zero_imm();
    cpu.registers[instruction.get_rd()] += imm;
}

void c_addiw(CPU& cpu, const CompressedInstruction& instruction)
{
    const u64 imm = instruction.get_none_zero_imm();
    cpu.registers[instruction.get_rd()] = (u64)(i64)(i32)(cpu.registers[instruction.get_rd()] + imm);
}

void c_li(CPU& cpu, const CompressedInstruction& instruction)
{
    const u64 imm = instruction.get_none_zero_imm();
    cpu.registers[instruction.get_rd()] = imm;
}

void c_lui(CPU& cpu, const CompressedInstruction& instruction)
{
    const u64 imm = instruction.get_lui_non_zero_imm();
    cpu.registers[instruction.get_rd()] = imm;
}

void c_addi16sp(CPU& cpu, const CompressedInstruction& instruction)
{
    const u64 imm = instruction.get_addi16sp_none_zero_imm();
    cpu.sp() = cpu.sp() + imm;
}

void c_addi4spn(CPU& cpu, const CompressedInstruction& instruction)
{
    const u64 imm = instruction.get_addi4spn_none_zero_unsigned_imm();
    cpu.registers[instruction.get_rd_alt()] = cpu.sp() + imm;
}

void c_slli(CPU& cpu, const CompressedInstruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rd()] << instruction.get_shamt();
}

void c_srli(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd_with_offset();
    cpu.registers[rd] >>= instruction.get_shamt();
}

void c_srai(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd_with_offset();
    cpu.registers[rd] = (u64)(i64)(i32)(cpu.registers[rd] >> instruction.get_shamt());
}

void c_andi(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd_with_offset();
    cpu.registers[rd] &= instruction.get_none_zero_imm();
}

void c_mv(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd();
    const u8 rs2 = instruction.get_rs2();
    cpu.registers[rd] = cpu.registers[rs2];
}

void c_add(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd();
    const u8 rs2 = instruction.get_rs2();
    cpu.registers[rd] += cpu.registers[rs2];
}

void c_addw(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rs1_alt();
    const u8 rs2 = instruction.get_rd_alt();
    cpu.registers[rd] = (u64)(i64)(i32)(cpu.registers[rd] + cpu.registers[rs2]);
}

void c_and(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd_with_offset();
    const u8 rs2 = instruction.get_rs2_alt();
    cpu.registers[rd] &= cpu.registers[rs2];
}

void c_or(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd_with_offset();
    const u8 rs2 = instruction.get_rs2_alt();
    cpu.registers[rd] |= cpu.registers[rs2];
}

void c_xor(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd_with_offset();
    const u8 rs2 = instruction.get_rs2_alt();
    cpu.registers[rd] ^= cpu.registers[rs2];
}


void c_sub(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd_with_offset();
    const u8 rs2 = instruction.get_rs2_alt();
    cpu.registers[rd] -= cpu.registers[rs2];
}

void c_subw(CPU& cpu, const CompressedInstruction& instruction)
{
    const u8 rd = instruction.get_rd_with_offset();
    const u8 rs2 = instruction.get_rs2_alt();
    cpu.registers[rd] = (u64)(i64)(i32)(cpu.registers[rd] - cpu.registers[rs2]);
}

void c_ebreak(CPU& cpu, const CompressedInstruction& instruction)
{
    cpu.raise_exception(Exception::Breakpoint, 0);
}
