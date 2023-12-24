#include "opcodes_m.h"

bool opcodes_m(CPU& cpu, const Instruction& instruction)
{
    const u64 opcode = instruction.get_opcode();
    const u64 funct3 = instruction.get_funct3();

    switch (opcode)
    {
        case OPCODES_M:
        {
            switch (funct3)
            {
                case MUL:       mul(cpu, instruction);    break;
                case MULH:      mulh(cpu, instruction);   break;
                case MULHSU:    mulhsu(cpu, instruction); break;
                case MULHU:     mulhu(cpu, instruction);  break;
                case DIV:       div(cpu, instruction);    break;
                case DIVU:      divu(cpu, instruction);   break;
                case REM:       rem(cpu, instruction);    break;
                case REMU:      remu(cpu, instruction);   break;
                default:        return false;
            }
            break;
        }

        case OPCODES_M_32:
        {
            switch (funct3)
            {
                case MULW:      mulw(cpu, instruction);   break;
                case DIVW:      divw(cpu, instruction);   break;
                case DIVUW:     divuw(cpu, instruction);  break;
                case REMW:      remw(cpu, instruction);   break;
                case REMUW:     remuw(cpu, instruction);  break;
                default:        return false;
            }
            break;
        }

        default: return false;
    }

    return true;
}

void mul(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        cpu.registers[instruction.get_rs1()] *
        cpu.registers[instruction.get_rs2()];
}

void mulh(CPU& cpu, const Instruction& instruction)
{

}

void mulhsu(CPU& cpu, const Instruction& instruction)
{

}

void mulhu(CPU& cpu, const Instruction& instruction)
{

}

void div(CPU& cpu, const Instruction& instruction)
{

}

void divu(CPU& cpu, const Instruction& instruction)
{

}

void rem(CPU& cpu, const Instruction& instruction)
{

}

void remu(CPU& cpu, const Instruction& instruction)
{

}

void mulw   (CPU& cpu, const Instruction& instruction)
{

}

void divw   (CPU& cpu, const Instruction& instruction)
{

}

void divuw(CPU& cpu, const Instruction& instruction)
{

}

void remw(CPU& cpu, const Instruction& instruction)
{

}

void remuw(CPU& cpu, const Instruction& instruction)
{

}

