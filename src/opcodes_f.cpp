#include "opcodes_f.h"
#include <cfenv>
#include <cmath>

/*
    Complying with IEEE 754 requires the following:
    - Asserting to make sure that it's supported
    - Compiling with -ffloat-store (and storing all intermediate results into variables)
*/
static_assert(std::numeric_limits<float>::is_iec559);

void init_opcodes_f()
{
    // Default RISC-V rounding mode (0 = RNE)
    std::fesetround(FE_TONEAREST);
}

bool opcodes_f(CPU& cpu, const Instruction instruction)
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
                case FSUB_S: fsub_s(cpu, instruction); return true;
                case FMUL_S: fmul_s(cpu, instruction); return true;
                case FDIV_S: fdiv_s(cpu, instruction); return true;

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
static inline void update_flags(F&& f, CPU& cpu, const Instruction instruction)
{
    // Clear host-CPU floating-point exceptions
    std::feclearexcept(FE_ALL_EXCEPT);


    f();

    // Query exceptions and update CSR accordingly
    if (std::fetestexcept(FE_INVALID)) cpu.fcsr.set_nv();
    if (std::fetestexcept(FE_DIVBYZERO)) cpu.fcsr.set_dz();
    if (std::fetestexcept(FE_OVERFLOW)) cpu.fcsr.set_of();
    if (std::fetestexcept(FE_UNDERFLOW)) cpu.fcsr.set_uf();
    if (std::fetestexcept(FE_INEXACT)) cpu.fcsr.set_nx();

    // Deal with NaN
    u32 quiet_nan = 0x7fc00000;
    auto& result = cpu.float_registers[instruction.get_rd()];
    if (std::isnan(result))
        memcpy(&result, &quiet_nan, sizeof(quiet_nan));
}

void flw(CPU& cpu, const Instruction instruction)
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

void fadd_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&](){
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.float_registers[instruction.get_rd()] = a + b;
    }, cpu, instruction);
}

void fsub_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&](){
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.float_registers[instruction.get_rd()] = a - b;
    }, cpu, instruction);
}

void fmul_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&](){
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.float_registers[instruction.get_rd()] = a * b;
    }, cpu, instruction);
}

void fdiv_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&](){
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.float_registers[instruction.get_rd()] = a / b;
    }, cpu, instruction);
}

void fmv_x_w(CPU& cpu, const Instruction instruction)
{
    u32 value = as_u32(cpu.float_registers[instruction.get_rs1()]);
    cpu.registers[instruction.get_rd()] = (i64)(i32)value;
}
