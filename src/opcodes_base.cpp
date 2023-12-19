#include "opcodes_base.h"
#include <stdexcept>
#include <iostream>

void opcode_base(CPU& cpu, const Instruction& instruction)
{
    u8 opcode = instruction.get_opcode();
    u8 funct3 = instruction.get_funct3();

    const auto unknown = [&]()
    {
        throw std::runtime_error(
            "unknown opcpode " +
            std::to_string(funct3) +
            " with funct3 " +
            std::to_string(funct3)
        );
    };

    switch(opcode)
    {
        case OPCODES_BASE_I_TYPE:
        {
            switch(funct3)
            {
                case ADDI: addi(cpu, instruction); break;

                default:
                    unknown();
            }
            break;
        }

        case JAL: jal(cpu, instruction); break;

        default:
            unknown();
    }
}

void addi(CPU& cpu, const Instruction& instruction)
{
    // No need for overflow checks
    cpu.registers[instruction.get_rs1()] = sign_extend_16(instruction.get_imm(Instruction::Type::I));
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
    // Add offset to program counter
    i64 offset = sign_extend_16(instruction.get_imm(Instruction::Type::J));
    cpu.pc += offset;

    // Target register will contain pc + 4
    cpu.registers[instruction.get_rd()] = cpu.pc + 4;

    // Minus 4 because 4 is always added anyway by caller
    cpu.pc -= 4;
}
