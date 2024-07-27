#include "opcodes_f.h"
#include <cfenv>
#include <cmath>

/*
    Complying with IEEE 754 requires the following:
    - Asserting to make sure that it's supported
    - Compiling with -ffloat-store (and storing all intermediate results into variables)
*/
static_assert(std::numeric_limits<float>::is_iec559);

constexpr u32 qNaN_float = 0x7fc00000; // a.k.a. "canconical" NaN
constexpr u32 sNaN_float = 0x7f800001;

#define ATTEMPTED_READ() \
    if (!check_fs_field(cpu, false)) return true;

#define ATTEMPTED_WRITE() \
    if (!check_fs_field(cpu, true)) return true;

#define CSR_CHANGED() cpu.mstatus.fields.fs = 3; dbg("changed");

void init_opcodes_f()
{
    // Default RISC-V rounding mode (0 = RNE)
    std::fesetround(FE_TONEAREST);
}

bool check_fs_field(CPU& cpu, bool is_write)
{
    // When an extension's status is set to off, any instruction that attempts
    // to read or write the corresponding state will cause an exception.
    if (cpu.mstatus.fields.fs == 0)
    {
        dbg("warning: attempt to use RV64F with mstatus.fs disabled");
        cpu.raise_exception(Exception::IllegalInstruction);
        return false;
    }

    // If we got this far, the floating-point extension is enabled, but there's
    // been some sort of attempt to modify its state.
    // See Table 3.4 of the privileged spec ("FS and XS state transitions")
    // In short, if it's a write we become "dirty".
    if (is_write)
        cpu.mstatus.fields.fs = 3;

    return true;
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
                case FLW: { ATTEMPTED_WRITE(); flw(cpu, instruction); return true; }
                default: return false;
            }
        }

        case OPCODES_F_2:
        {
            switch (funct3)
            {
                case FSW: { ATTEMPTED_READ(); fsw(cpu, instruction); return true; }
                default: return false;
            }
        }

        case OPCODES_F_3:
        {
            switch (funct2)
            {
                case FMADD_S: { ATTEMPTED_WRITE(); fmadd_s(cpu, instruction); return true; }
                default: return false;
            }
        }

        case OPCODES_F_4:
        {
            switch (funct2)
            {
                case FMSUB_S: { ATTEMPTED_WRITE(); fmsub_s(cpu, instruction); return true; }
                default: return false;
            }
        }

        case OPCODES_F_5:
        {
            switch (funct2)
            {
                case FNMADD_S: { ATTEMPTED_WRITE(); fnmadd_s(cpu, instruction); return true; }
                default: return false;
            }
        }

        case OPCODES_F_6:
        {
            switch (funct2)
            {
                case FNMSUB_S: { ATTEMPTED_WRITE(); fnmsub_s(cpu, instruction); return true; }
                default: return false;
            }
        }

        case OPCODES_F_7:
        {
            switch (funct7)
            {
                case FADD_S: { ATTEMPTED_WRITE(); fadd_s(cpu, instruction); return true; }
                case FSUB_S: { ATTEMPTED_WRITE(); fsub_s(cpu, instruction); return true; }
                case FMUL_S: { ATTEMPTED_WRITE(); fmul_s(cpu, instruction); return true; }
                case FDIV_S: { ATTEMPTED_WRITE(); fdiv_s(cpu, instruction); return true; }

                case 0x10:
                {
                    switch (funct3)
                    {
                        case FSGNJ_S:  { ATTEMPTED_WRITE(); fsgnj_s(cpu, instruction);  return true; }
                        case FSGNJN_S: { ATTEMPTED_WRITE(); fsgnjn_s(cpu, instruction); return true; }
                        case FSGNJX_S: { ATTEMPTED_WRITE(); fsgnjx_s(cpu, instruction); return true; }
                        default: return false;
                    }
                }

                case 0x14:
                {
                    switch (funct3)
                    {
                        case FMIN_S: { ATTEMPTED_WRITE(); fmin_s(cpu, instruction); return true; }
                        case FMAX_S: { ATTEMPTED_WRITE(); fmax_s(cpu, instruction); return true; }
                        default: return false;
                    }
                }

                case 0x50:
                {
                    switch (funct3)
                    {
                        case FEQ_S: { ATTEMPTED_READ(); feq_s(cpu, instruction); return true; }
                        case FLT_S: { ATTEMPTED_READ(); flt_s(cpu, instruction); return true; }
                        case FLE_S: { ATTEMPTED_READ(); fle_s(cpu, instruction); return true; }
                        default: return false;
                    }
                }

                case 0x60:
                {
                    switch (instruction.get_rs2())
                    {
                        case FCVT_W_S:  { ATTEMPTED_READ(); fcvt_w_s(cpu, instruction);  return true; }
                        case FCVT_L_S:  { ATTEMPTED_READ(); fcvt_l_s(cpu, instruction);  return true; }
                        case FCVT_WU_S: { ATTEMPTED_READ(); fcvt_wu_s(cpu, instruction); return true; }
                        case FCVT_LU_S: { ATTEMPTED_READ(); fcvt_lu_s(cpu, instruction); return true; }
                        default: return false;
                    }
                }

                case 0x68:
                {
                    if (!check_fs_field(cpu, true)) return true;

                    switch (instruction.get_rs2())
                    {
                        case FCVT_S_W:  { ATTEMPTED_WRITE(); fcvt_s_w(cpu, instruction);  return true; }
                        case FCVT_S_L:  { ATTEMPTED_WRITE(); fcvt_s_l(cpu, instruction);  return true; }
                        case FCVT_S_WU: { ATTEMPTED_WRITE(); fcvt_s_wu(cpu, instruction); return true; }
                        case FCVT_S_LU: { ATTEMPTED_WRITE(); fcvt_s_lu(cpu, instruction); return true; }
                        default: return false;
                    }
                }

                case 0x70:
                {
                    if (!check_fs_field(cpu, false)) return true;

                    switch (funct3)
                    {
                        case FMV_X_W:  { ATTEMPTED_READ(); fmv_x_w(cpu, instruction);  return true; }
                        case FCLASS_S: { ATTEMPTED_READ(); fclass_s(cpu, instruction); return true; }
                        default: return false;
                    }
                }

                case FMV_W_X: { ATTEMPTED_WRITE(); fmv_w_x(cpu, instruction); return true; }
                case FSQRT_S: { ATTEMPTED_READ(); fsqrt_s(cpu, instruction); return true; }
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
static inline void update_flags(F&& f, CPU& cpu, const Instruction instruction, const bool use_rs1 = false)
{
    // Clear host-CPU floating-point exceptions
    std::feclearexcept(FE_ALL_EXCEPT);

    f();

    // Query exceptions and update CSR accordingly
    if (std::fetestexcept(FE_INVALID))   cpu.fcsr.set_nv(cpu);
    if (std::fetestexcept(FE_DIVBYZERO)) cpu.fcsr.set_dz(cpu);
    if (std::fetestexcept(FE_OVERFLOW))  cpu.fcsr.set_of(cpu);
    if (std::fetestexcept(FE_UNDERFLOW)) cpu.fcsr.set_uf(cpu);
    if (std::fetestexcept(FE_INEXACT))   cpu.fcsr.set_nx(cpu);

    // Deal with NaN by overwriting result of f() with NaN
    const size_t index = use_rs1 ? instruction.get_rs1() : instruction.get_rd();
    if (std::isnan(cpu.float_registers[index]))
    {
        float qNaN;
        memcpy(&qNaN, &qNaN_float, sizeof(qNaN));
        cpu.float_registers[index] = qNaN;
    }
}

static inline void set_rounding_mode(const CPU& cpu, const Instruction instruction)
{
    FCSR::RoundingMode mode = (FCSR::RoundingMode)instruction.get_rounding_mode();
    if (mode == FCSR::RoundingMode::DYNAMIC)
        mode = cpu.fcsr.get_rounding_mode();

    switch (mode)
    {
        case FCSR::RoundingMode::RNE:
            fesetround(FE_TONEAREST);
            break;

        case FCSR::RoundingMode::RTZ:
            fesetround(FE_TOWARDZERO);
            break;

        case FCSR::RoundingMode::RDN:
            fesetround(FE_DOWNWARD);
            break;

        case FCSR::RoundingMode::RUP:
            fesetround(FE_UPWARD);
            break;

        case FCSR::RoundingMode::RMM:
            fesetround(FE_TONEAREST);
            break;

        default:
            throw std::runtime_error("unsupported rounding mode " + std::to_string(mode));
    }
}

template<typename T, typename U>
static void round_float(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);

    float result;
    update_flags([&]()
    {
        // NOTE: Using std::rintf is required to respect the rounding mode.
        //       It is preferable to std::nearbyint as it will raise FE_INEXACT
        //       for us.
        result = std::rintf(cpu.float_registers[instruction.get_rs1()]);
    }, cpu, instruction, true);

    // If the rounded result is not representable in the destination
    // format, it is clipped to the nearest value and the invalid flag
    // is set. NaN is always treated as positive.
    if (result > std::numeric_limits<T>::max() || std::isnan(result))
    {
        result = std::numeric_limits<T>::max();
        cpu.fcsr.set_nv(cpu);
    }

    else if (result < std::numeric_limits<T>::min())
    {
        result = std::numeric_limits<T>::min();
        cpu.fcsr.set_nv(cpu);
    }

    cpu.registers[instruction.get_rs1()] = (U)(T)result;
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

void fsw(CPU& cpu, const Instruction instruction)
{
    const u64 address = instruction.get_imm(Instruction::Type::S) +
        cpu.registers[instruction.get_rs1()];

    const float value = cpu.float_registers[instruction.get_rs2()];

    const auto error = cpu.write_32(address, as_u32(value));
    if (error.has_value())
        cpu.raise_exception(*error);
}

void fmadd_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        const float c = cpu.float_registers[instruction.get_rs3()];
        cpu.float_registers[instruction.get_rd()] = a * b + c;
    }, cpu, instruction);
}

void fmsub_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        const float c = cpu.float_registers[instruction.get_rs3()];
        cpu.float_registers[instruction.get_rd()] = a * b - c;
    }, cpu, instruction);
}

void fnmadd_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        const float c = cpu.float_registers[instruction.get_rs3()];
        cpu.float_registers[instruction.get_rd()] = -a * b + c;
    }, cpu, instruction);
}

void fnmsub_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        const float c = cpu.float_registers[instruction.get_rs3()];
        cpu.float_registers[instruction.get_rd()] = -a * b - c;
    }, cpu, instruction);
}

void fadd_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.float_registers[instruction.get_rd()] = a + b;
    }, cpu, instruction);
}

void fsub_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.float_registers[instruction.get_rd()] = a - b;
    }, cpu, instruction);
}

void fmul_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.float_registers[instruction.get_rd()] = a * b;
    }, cpu, instruction);
}

void fdiv_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.float_registers[instruction.get_rd()] = a / b;
    }, cpu, instruction);
}

void fsqrt_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        cpu.float_registers[instruction.get_rd()] = std::sqrtf(a);
    }, cpu, instruction);
}

void fsgnj_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);

    // Copy all bits (except the sign bit) from rs1, and use the sign bit of rs2
    const float a = cpu.float_registers[instruction.get_rs1()];
    const float b = cpu.float_registers[instruction.get_rs2()];
    cpu.float_registers[instruction.get_rd()] = std::copysignf(std::fabs(a), b);
}

void fsgnjn_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);

    // As above but rs2's sign is inverted
    const float a = cpu.float_registers[instruction.get_rs1()];
    const float b = cpu.float_registers[instruction.get_rs2()];
    cpu.float_registers[instruction.get_rd()] = std::copysignf(std::fabs(a), -b);
}

void fsgnjx_s(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);

    // The sign bit is the XOR of the two sign bits
    const u32 a = as_u32(cpu.float_registers[instruction.get_rs1()]);
    const u32 b = as_u32(cpu.float_registers[instruction.get_rs2()]);
    const u32 sign_a = a & 0x80000000;
    const u32 sign_b = b & 0x80000000;
    const u32 abs_a = a & 0x7fffffff;
    const u32 result = (sign_a ^ sign_b) | abs_a;
    cpu.float_registers[instruction.get_rd()] = as_float(result);
}

void fmin_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];

        const auto get_min = [&]()
        {
            // fmin(+0, -0) = -0
            if (a == 0.0f && b == 0.0f)
            {
                if (std::signbit(a) || std::signbit(b)) return -0.0f;
                else return 0.0f;
            }

            return std::fmin(a, b);
        };

        cpu.float_registers[instruction.get_rd()] = get_min();
    }, cpu, instruction);
}

void fmax_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];

        const auto get_max = [&]()
        {
            // fmax(+0, -0) = +0
            if (a == 0.0f && b == 0.0f)
            {
                if (std::signbit(a) && std::signbit(b)) return -0.0f;
                else return 0.0f;
            }

            // fmax(sNaN, x) = x
            if (as_u32(a) == sNaN_float) { cpu.fcsr.set_nv(cpu); return b; }
            if (as_u32(b) == sNaN_float) { cpu.fcsr.set_nv(cpu); return a; }

            return std::fmax(a, b);
        };

        cpu.float_registers[instruction.get_rd()] = get_max();
    }, cpu, instruction);
}

void fcvt_s_w(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    cpu.float_registers[instruction.get_rd()] =
        (float)(i32)cpu.registers[instruction.get_rs1()];
}

void fcvt_s_l(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    cpu.float_registers[instruction.get_rd()] =
        (double)(float)(i32)cpu.registers[instruction.get_rs1()];
}

void fcvt_s_wu(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    cpu.float_registers[instruction.get_rd()] =
        (float)(u32)cpu.registers[instruction.get_rs1()];
}

void fcvt_s_lu(CPU& cpu, const Instruction instruction)
{
    set_rounding_mode(cpu, instruction);
    cpu.float_registers[instruction.get_rd()] =
        (double)(float)cpu.registers[instruction.get_rs1()];
}

void fcvt_w_s(CPU& cpu, const Instruction instruction)
{
    round_float<i32, i32>(cpu, instruction);
}

void fcvt_l_s(CPU& cpu, const Instruction instruction)
{
    round_float<i64, i64>(cpu, instruction);
}

void fcvt_wu_s(CPU& cpu, const Instruction instruction)
{
    round_float<u32, i32>(cpu, instruction);
}

void fcvt_lu_s(CPU& cpu, const Instruction instruction)
{
    round_float<u64, u64>(cpu, instruction);
}

void fmv_x_w(CPU& cpu, const Instruction instruction)
{
    // Unaffected by rounding mode despite having RM field
    u32 value = as_u32(cpu.float_registers[instruction.get_rs1()]);
    cpu.registers[instruction.get_rd()] = (i64)(i32)value;
}

void fmv_w_x(CPU& cpu, const Instruction instruction)
{
    // Unaffected by rounding mode despite having RM field
    float value = as_float(cpu.registers[instruction.get_rs1()]);
    cpu.float_registers[instruction.get_rd()] = value;
}

void feq_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.registers[instruction.get_rd()] = (a == b) ? 1 : 0;
    }, cpu, instruction);
}

void flt_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.registers[instruction.get_rd()] = (a < b) ? 1 : 0;
    }, cpu, instruction);
}

void fle_s(CPU& cpu, const Instruction instruction)
{
    update_flags([&]()
    {
        const float a = cpu.float_registers[instruction.get_rs1()];
        const float b = cpu.float_registers[instruction.get_rs2()];
        cpu.registers[instruction.get_rd()] = (a <= b) ? 1 : 0;
    }, cpu, instruction);
}

void fclass_s(CPU& cpu, const Instruction instruction)
{
    const float value = cpu.float_registers[instruction.get_rs1()];
    u64& rd = cpu.registers[instruction.get_rd()];

    switch (std::fpclassify(value))
    {
        case FP_INFINITE:
        {
            // Negative infinity
            if (value == -std::numeric_limits<float>::infinity())
                rd = 0b1;

            // Positive infinity
            else if (value == std::numeric_limits<float>::infinity())
                rd = 0b10000000;

            break;
        }

        case FP_ZERO:
        {
            // Minus zero
            if (std::signbit(value))
                rd = 0b1000;

            // Plus zero
            else
                rd = 0b10000;

            break;
        }

        case FP_NAN:
        {
            // Quiet NaN
            if (as_u32(value) == qNaN_float)
                rd = 0b1000000000;

            // Signaling NaN (we hope)
            else
                rd = 0b100000000;

            break;
        }

        case FP_SUBNORMAL:
        {
            // Negative subnormal
            if (std::signbit(value))
                rd = 0b100;

            // Positive subnormal
            else
                rd = 0b100000;

            break;
        }

        default:
        {
            // Negative normal
            if (std::signbit(value))
                rd = 0b10;

            // Positive normal
            else
                rd = 0b1000000;

            break;
        }
    }
}
