#include "opcodes_f.h"
#include <cfenv>

bool opcodes_f(CPU& cpu, const Instruction& instruction)
{
    const u8 opcode = instruction.get_opcode();
    const u8 funct3 = instruction.get_funct3();
    const u8 funct7 = instruction.get_funct7();

    switch (opcode)
    {
        case OPCODES_F_1:
        {
            switch (funct3)
            {
                case FLW: flw(cpu, instruction); return true;
                default: return false;
            }
        }

        case OPCODES_F_7:
        {
            switch (funct7)
            {
                case FADD_S: fadd_s(cpu, instruction); return true;

                case 0x70:
                {
                    switch (funct3)
                    {
                        case FMV_X_W: fmv_x_w(cpu, instruction); return true;
                        default: return false;
                    }
                }

                default:                                    return false;
            }
        }

        default: return false;
    }

    return false;
}

static inline float as_float(const u32 bits)
{
    float* f = (float*)&bits;
    return *f;
}

static inline u32 as_u32(const float f)
{
    u32* u = (u32*)&f;
    return *u;
}

template<typename F>
void update_flags(F&& f, CPU& cpu)
{
    // Clear host-CPU floating-point exceptions
    std::feclearexcept(FE_ALL_EXCEPT);

    f();

    // Query exceptions and update CSR accordingly
    if (fetestexcept(FE_INVALID)) cpu.fcsr.set_nv();
    if (fetestexcept(FE_DIVBYZERO)) cpu.fcsr.set_dz();
    if (fetestexcept(FE_OVERFLOW)) cpu.fcsr.set_of();
    if (fetestexcept(FE_UNDERFLOW)) cpu.fcsr.set_uf();
    if (fetestexcept(FE_INEXACT)) cpu.fcsr.set_nx();
}

void flw(CPU& cpu, const Instruction& instruction)
{
    const u64 address = instruction.get_imm(Instruction::Type::I) +
        cpu.registers[instruction.get_rs1()];

    const auto value = cpu.read_32(address);
    if (!value)
    {
        cpu.raise_exception(value.error());
        return;
    }

    cpu.float_registers[instruction.get_rd()] = as_float(*value);
}

void fadd_s(CPU& cpu, const Instruction& instruction)
{
    update_flags([&](){
        cpu.float_registers[instruction.get_rd()] =
            cpu.float_registers[instruction.get_rs1()] +
            cpu.float_registers[instruction.get_rs2()];
    }, cpu);
}

void fmv_x_w(CPU& cpu, const Instruction& instruction)
{
    u32 value = as_u32(cpu.float_registers[instruction.get_rs1()]);
    cpu.registers[instruction.get_rd()] = (i64)(i32)value;
}
