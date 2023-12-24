#include "opcodes_m.h"
#include <limits>

// Helpers
template<typename T>
bool overflow(T dividend, T divisor);
void divide_by_zero(CPU& cpu, const Instruction& instruction);

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
        (i64)cpu.registers[instruction.get_rs1()] *
        (i64)cpu.registers[instruction.get_rs2()];
}

void mulh(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        ((i128)(i64)cpu.registers[instruction.get_rs1()] *
        (i128)(i64)cpu.registers[instruction.get_rs2()]) >> 64;
}

void mulhsu(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        ((i128)(i64)cpu.registers[instruction.get_rs1()] *
        (u128)cpu.registers[instruction.get_rs2()]) >> 64;
}

void mulhu(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] =
        ((u128)cpu.registers[instruction.get_rs1()] *
        (u128)cpu.registers[instruction.get_rs2()]) >> 64;
}

void div(CPU& cpu, const Instruction& instruction)
{
    const i64 dividend = (i64)cpu.registers[instruction.get_rs1()];
    const i64 divisor = (i64)cpu.registers[instruction.get_rs2()];

    if (divisor == 0)
        divide_by_zero(cpu, instruction);

    else if (overflow<i64>(dividend, divisor))
        cpu.registers[instruction.get_rd()] = dividend;

    else
        cpu.registers[instruction.get_rd()] =  dividend / divisor;
}

void divu(CPU& cpu, const Instruction& instruction)
{
    const u64 dividend = cpu.registers[instruction.get_rs1()];
    const u64 divisor = cpu.registers[instruction.get_rs2()];

    if (divisor == 0) divide_by_zero(cpu, instruction);
    else
        cpu.registers[instruction.get_rd()] =  dividend / divisor;
}

void rem(CPU& cpu, const Instruction& instruction)
{

}

void remu(CPU& cpu, const Instruction& instruction)
{

}

void mulw(CPU& cpu, const Instruction& instruction)
{
    cpu.registers[instruction.get_rd()] = i64(
        (i32)cpu.registers[instruction.get_rs1()] *
        (i32)cpu.registers[instruction.get_rs2()]
    );
}

void divw(CPU& cpu, const Instruction& instruction)
{
    const i32 dividend = (i32)cpu.registers[instruction.get_rs1()];
    const i32 divisor = (i32)cpu.registers[instruction.get_rs2()];

    if (divisor == 0)
        divide_by_zero(cpu, instruction);

    else if (overflow<i32>(dividend, divisor))
        cpu.registers[instruction.get_rd()] = (i64)dividend;

    else
        cpu.registers[instruction.get_rd()] =  (i64)(dividend / divisor);
}

void divuw(CPU& cpu, const Instruction& instruction)
{
    const u32 dividend = (u32)cpu.registers[instruction.get_rs1()];
    const u32 divisor = (u32)cpu.registers[instruction.get_rs2()];

    if (divisor == 0)
        divide_by_zero(cpu, instruction);

    else
        cpu.registers[instruction.get_rd()] =  (i64)(i32)(dividend / divisor);
}

void remw(CPU& cpu, const Instruction& instruction)
{

}

void remuw(CPU& cpu, const Instruction& instruction)
{

}

template<typename T>
bool overflow(T dividend, T divisor)
{
    return dividend == std::numeric_limits<T>::min() && divisor == -1;
}

void divide_by_zero(CPU& cpu, const Instruction& instruction)
{
    // TODO: set DZ flag to 1 when floating point support added?
    cpu.registers[instruction.get_rd()] = std::numeric_limits<u64>::max();
}
