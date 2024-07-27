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
    const u8 funct2 = instruction.get_funct2();
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

        case OPCODES_F_2:
        {
            switch (funct3)
            {
                case FSW: fsw(cpu, instruction); return true;
                default: return false;
            }
        }

        case OPCODES_F_3:
        {
            switch (funct2)
            {
                case FMADD_S: fmadd_s(cpu, instruction); return true;
                default: return false;
            }
        }

        case OPCODES_F_4:
        {
            switch (funct2)
            {
                case FMSUB_S: fmsub_s(cpu, instruction); return true;
                default: return false;
            }
        }

        case OPCODES_F_5:
        {
            switch (funct2)
            {
                case FNMADD_S: fnmadd_s(cpu, instruction); return true;
                default: return false;
            }
        }

        case OPCODES_F_6:
        {
            switch (funct2)
            {
                case FNMSUB_S: fnmsub_s(cpu, instruction); return true;
                default: return false;
            }
        }

        case OPCODES_F_7:
        {
            dbg("todo: check frm field is valid");

            switch (funct7)
            {
                case FADD_S: fadd_s(cpu, instruction); return true;
                case FSUB_S: fsub_s(cpu, instruction); return true;
                case FMUL_S: fmul_s(cpu, instruction); return true;
                case FDIV_S: fdiv_s(cpu, instruction); return true;

                case 0x10:
                {
                    switch (funct3)
                    {
                        case FSGNJ_S:  fsgnj_s(cpu, instruction);  return true;
                        case FSGNJN_S: fsgnjn_s(cpu, instruction); return true;
                        case FSGNJX_S: fsgnjx_s(cpu, instruction); return true;
                        default: return false;
                    }
                }

                case 0x14:
                {
                    switch (funct3)
                    {
                        case FMIN_S: fmin_s(cpu, instruction); return true;
                        case FMAX_S: fmax_s(cpu, instruction); return true;
                        default: return false;
                    }
                }

                case 0x50:
                {
                    switch (funct3)
                    {
                        case FEQ_S: feq_s(cpu, instruction); return true;
                        case FLT_S: flt_s(cpu, instruction); return true;
                        case FLE_S: fle_s(cpu, instruction); return true;
                        default: return false;
                    }
                }

                case 0x60:
                {
                    switch (instruction.get_rs2())
                    {
                        case FCVT_W_S:  fcvt_w_s(cpu, instruction);  return true;
                        case FCVT_WU_S: fcvt_wu_s(cpu, instruction); return true;
                        default: return false;
                    }
                }

                case 0x68:
                {
                    switch (instruction.get_rs2())
                    {
                        case FCVT_S_W:  fcvt_s_w(cpu, instruction);  return true;
                        case FCVT_S_WU: fcvt_s_wu(cpu, instruction); return true;
                        default: return false;
                    }
                }

                case 0x70:
                {
                    switch (funct3)
                    {
                        case FMV_X_W:  fmv_x_w(cpu, instruction);  return true;
                        case FCLASS_S: fclass_s(cpu, instruction); return true;
                        default: return false;
                    }
                }

                case FMV_W_X:     fmv_w_x(cpu, instruction); return true;
                case FDIV_SQRT_S: fsqrt_s(cpu, instruction); return true;

                default: return false;
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

void fsqrt_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&](){
        const float a = cpu.float_registers[instruction.get_rs1()];
        cpu.float_registers[instruction.get_rd()] = sqrtf(a);
    }, cpu, instruction);
}

void fmv_x_w(CPU& cpu, const Instruction instruction)
{
    u32 value = as_u32(cpu.float_registers[instruction.get_rs1()]);
    cpu.registers[instruction.get_rd()] = (i64)(i32)value;
}
