#include "opcodes_base.h"
#include <stdexcept>
#include <iostream>

bool opcodes_base(CPU& cpu, const Instruction& instruction)
{
    u8 opcode = instruction.get_opcode();
    u8 funct3 = instruction.get_funct3();

    switch(opcode)
    {
        case OPCODES_BASE_I_TYPE:
        {
            switch(funct3)
            {
                case ADDI: addi(cpu, instruction); break;

                default:
                    return false;
            }
            break;
        }

        case JAL: jal(cpu, instruction); break;

        default:
            return false;
    }

    return true;
}

void addi(CPU& cpu, const Instruction& instruction)
{
    // No need for overflow checks
    i64 imm = instruction.get_imm(Instruction::Type::I);
    cpu.registers[instruction.get_rd()] = cpu.registers[instruction.get_rs1()] + imm;
}

void slli(CPU& cpu, const Instruction& instruction) {}
void slti(CPU& cpu, const Instruction& instruction) {}
void sltiu(CPU& cpu, const Instruction& instruction) {}
void xori(CPU& cpu, const Instruction& instruction) {}
void sri(CPU& cpu, const Instruction& instruction) {}
void srli(CPU& cpu, const Instruction& instruction) {}
void srai(CPU& cpu, const Instruction& instruction) {}
void ori(CPU& cpu, const Instruction& instruction) {}
void andi(CPU& cpu, const Instruction& instruction) {}

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
