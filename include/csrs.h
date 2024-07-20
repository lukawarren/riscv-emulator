#pragma once
#include "common.h"

#define CSR_SSTATUS         0x100
#define CSR_SIE             0x104
#define CSR_STVEC           0x105
#define CSR_SCOUNTER_EN     0x106
#define CSR_SSCRATCH        0x140
#define CSR_SEPC            0x141
#define CSR_SCAUSE          0x142
#define CSR_STVAL           0x143
#define CSR_SIP             0x144
#define CSR_SATP            0x180
#define CSR_MSTATUS         0x300
#define CSR_MISA            0x301
#define CSR_MEDELEG         0x302
#define CSR_MIDELEG         0x303
#define CSR_MIE             0x304
#define CSR_MTVEC           0x305
#define CSR_MCOUNTER_EN     0x306
#define CSR_MSCRATCH        0x340
#define CSR_MEPC            0x341
#define CSR_MCAUSE          0x342
#define CSR_MTVAL           0x343
#define CSR_MIP             0x344
#define CSR_MTINST          0x34a
#define CSR_MTVAL2          0x34b
#define CSR_PMPCFG0         0x3a0
#define CSR_PMPCFG15        0x3af
#define CSR_PMPADDR0        0x3b0
#define CSR_PMPADDR63       0x3ef
#define CSR_MNSTATUS        0x744
#define CSR_DEBUG_BEGIN     0x7a0
#define CSR_TDATA1          0x7a1
#define CSR_DEBUG_LIMIT     0x7af
#define CSR_DEBUG_END       0x7bf
#define CSR_MCYCLE          0xb00
#define CSR_MINSTRET        0xb02
#define CSR_MHPMCOUNTER3    0xb03
#define CSR_MHPMCOUNTER31   0xb1f
#define CSR_CYCLE           0xc00
#define CSR_TIME            0xc01
#define CSR_INSTRET         0xc02
#define CSR_MVENDOR_ID      0xf11
#define CSR_MARCH_ID        0xf12
#define CSR_MIMP_ID         0xf13
#define CSR_MHART_ID        0xf14

enum class PrivilegeLevel
{
    User = 0,
    Supervisor = 1,
    Hypervisor = 2,
    Machine = 3,
    Debug = 4
};

class CPU;

/*
    Each CSR is 12 bits:
    - Last 4 bits control R/W access and privilege level
    - 2 most significant bits determine R/W status
    - Next 2 most significant bits determine privilege

    - Attempts to access a non-existent CSR raise an illegal instruction exception.

    - Attempts to access a CSR without appropriate privilege level or to write a
        read-only register also raise illegal instruction
        exceptions.

    - A read/write register might also contain some bits that are read-only, in which case
        writes to the read-only bits are ignored.


    Field specifications
    --------------------

    - WPRI (reserved writes preserve values, reads ignore values):
        - Field is reserved for future use. Software should ignore these values,
          and should preserve the values held in these fields when writing values
          to other fields of the same register.

        - Implementations that do not furnish these fields must make
          them *read-only zero*.

    - WLRL (write/read only legal values):
        - Implementations are permitted but not required to raise an illegal
          instruction exception if an instruction attempts to write a
          non-supported value to a WLRL field

        - Software should not assume a read will return a legal value unless
          the last write was a legal value or another operation has set the
          register to a legal value

    - WARL (write any values, reads legal values):
        - Only defined for a subset of bit encodings, but allows any value to be
          written while guaranteeing to return a legal value whenever read

        - Assuming that writing the CSR has no other side effects, the range of
          supported values can be determined by attempting to write a desired
          setting then reading to see if the value was retained

        - Implementations will not raise an exception on writes of unsupported
          values to a WARL field
 */

struct CSR
{
    static bool is_read_only(const u16 address)
    {
        return ((address >> 10) & 0b11) == 0b11;
    }

    static PrivilegeLevel get_privilege_level(const u16 address)
    {
        /*
            Machine-mode standard read-write CSRs 0x7A0–0x7BF are reserved for use by
            the debug system. Of these CSRs, 0x7A0–0x7AF are accessible to machine mode,
            whereas 0x7B0–0x7BF are only visible to debug mode. Implementations should
            raise illegal instruction exceptions on machine-mode access to the latter
            set of registers.
         */
        if (address >= 0x7b0 && address <= 0x7bf)
            return PrivilegeLevel::Debug;

        const u8 code = (address >> 8) & 0b11;
        switch (code)
        {
            case 0b00: return PrivilegeLevel::User;
            case 0b01: return PrivilegeLevel::Supervisor;
            case 0b10: return PrivilegeLevel::Hypervisor;
            case 0b11: return PrivilegeLevel::Machine;
            default:   return PrivilegeLevel::Machine;
        }
    }

    bool increment(CPU& cpu)
    {
        std::optional<u64> value = read(cpu);
        if (!value) return false;
        return write(*value + 1, cpu);
    }

    virtual bool write(const u64 value, CPU& cpu) = 0;
    virtual std::optional<u64> read(CPU& cpu) = 0;
};

// No special restrictions or bits; just holds a value
struct DefaultCSR: CSR
{
    u64 value = 0;

    bool write(const u64 value, CPU&) override
    {
        this->value = value;
        return true;
    }

    std::optional<u64> read(CPU&) override {
        return value;
    }
};

struct MTVec : CSR
{
    u64 address = 0;

    // WARL
    enum class Mode
    {
        Direct = 0,
        Vectored = 1
    } mode;

    bool write(const u64 value, CPU&) override
    {
        address = value & 0xfffffffffffffffc;
        mode = (Mode)(value & 0b11);

        // WARL for mode; >=2 is reserved
        if (mode > Mode::Vectored)
            mode = Mode::Vectored;

        return true;
    }

    std::optional<u64> read(CPU&) override
    {
        return address | (u64)mode;
    }

    Mode get_mode()
    {
        if ((address & 1) == 0) return Mode::Direct;
        else return Mode::Vectored;
    }
};

// Does not shadow MTVec, but has the same format
struct STVec : MTVec {};

struct MCounterEnable : DefaultCSR
{
    bool is_hardware_performance_monitor_enabled(const u32 number) const
    {
        return ((value >> number) & 0b1) == 1;
    }

    bool is_cycle_enabled() const
    {
        return is_hardware_performance_monitor_enabled(0);
    }

    bool is_time_enabled() const
    {
        return is_hardware_performance_monitor_enabled(1);
    }

    bool is_instret_enabled() const
    {
        return is_hardware_performance_monitor_enabled(2);
    }
};

struct MEPC : CSR
{
    u64 address = 0;

    bool write(const u64 value, CPU&) override
    {
        // WARL; lowest bit is always zero, and 2nd lowest is zero if IALIGN
        // can only be 32 (but we support 16, or will)
        address = value & 0xfffffffffffffffe;
        return true;
    }

    std::optional<u64> read(CPU&) override
    {
        // Whenever IALIGN=32, bit mepc[1] is masked on reads so that it appears
        // to be 0. This masking occurs also for the implicit read by the MRET instruction.
        // Though masked, mepc[1] remains writable when IALIGN=32.
        if ((address & 0b10) != 0)
            std::cout << "TODO: respect IALIGN when adding C support" << std::endl;
        return address;
    }
};

struct SEPC : MEPC {};

struct MStatus : CSR
{
    struct Fields
    {
        u64 sd:     1;
        u64 wpri_1: 25;
        u64 mbe:    1;
        u64 sbe:    1;
        u64 sxl:    2; // warl
        u64 uxl:    2; // warl
        u64 wpri_2: 9;
        u64 tsr:    1;
        u64 tw:     1;
        u64 tvm:    1;
        u64 mxr:    1;
        u64 sum:    1;
        u64 mprv:   1;
        u64 xs:     2;
        u64 fs:     2;
        u64 mpp:    2;
        u64 vs:     2;
        u64 spp:    1;
        u64 mpie:   1;
        u64 ube:    1;
        u64 spie:   1;
        u64 wpri_3: 1;
        u64 mie:    1;
        u64 wpri_4: 1;
        u64 sie:    1;
        u64 wpri_5: 1;
    } fields;

    MStatus()
    {
        memset(&fields, 0, sizeof(Fields));
    }

    bool write(const u64 value, CPU&) override
    {
        // Don't set the wpri fields; keep them zero
        fields.sd = (value >> 63) & 0x1;
        fields.mbe = (value >> 37) & 0x1;
        fields.sbe = (value >> 36) & 0x1;
        fields.sxl = (value >> 34) & 0x3;
        fields.uxl = (value >> 32) & 0x3;
        fields.tsr = (value >> 22) & 0x1;
        fields.tw = (value >> 21) & 0x1;
        fields.tvm = (value >> 20) & 0x1;
        fields.mxr = (value >> 19) & 0x1;
        fields.sum = (value >> 18) & 0x1;
        fields.mprv = (value >> 17) & 0x1;
        fields.xs = (value >> 15) & 0x3;
        fields.fs = (value >> 13) & 0x3;
        fields.mpp = (value >> 11) & 0x3;
        fields.vs = (value >> 9) & 0x3;
        fields.spp = (value >> 8) & 0x1;
        fields.mpie = (value >> 7) & 0x1;
        fields.ube = (value >> 6) & 0x1;
        fields.spie = (value >> 5) & 0x1;
        fields.mie = (value >> 3) & 0x1;
        fields.sie = (value >> 1) & 0x1;

        // WARL for SXL and UXL
        fields.sxl = 2; // xlen = 64
        fields.uxl = 2; // xlen = 64

        return true;
    }

    std::optional<u64> read(CPU&) override
    {
        u64 value = 0;
        value |= ((u64)(fields.sd) << 63);
        value |= ((u64)(fields.wpri_1) << 38);
        value |= ((u64)(fields.mbe) << 37);
        value |= ((u64)(fields.sbe) << 36);
        value |= ((u64)(fields.sxl) << 34);
        value |= ((u64)(fields.uxl) << 32);
        value |= ((u64)(fields.wpri_2) << 23);
        value |= ((u64)(fields.tsr) << 22);
        value |= ((u64)(fields.tw) << 21);
        value |= ((u64)(fields.tvm) << 20);
        value |= ((u64)(fields.mxr) << 19);
        value |= ((u64)(fields.sum) << 18);
        value |= ((u64)(fields.mprv) << 17);
        value |= ((u64)(fields.xs) << 15);
        value |= ((u64)(fields.fs) << 13);
        value |= ((u64)(fields.mpp) << 11);
        value |= ((u64)(fields.vs) << 9);
        value |= ((u64)(fields.spp) << 8);
        value |= ((u64)(fields.mpie) << 7);
        value |= ((u64)(fields.ube) << 6);
        value |= ((u64)(fields.spie) << 5);
        value |= ((u64)(fields.wpri_3) << 4);
        value |= ((u64)(fields.mie) << 3);
        value |= ((u64)(fields.wpri_4) << 2);
        value |= ((u64)(fields.sie) << 1);
        value |= (u64)(fields.wpri_5);
        return value;
    }

    static_assert(sizeof(Fields) == sizeof(u64));
};

struct SStatus: CSR
{
    // "In a straightforward implementation, reading or writing any
    //  field in sstatus is equivalent to reading or writing the homonymous
    //  field in mstatus."
    // Hence there is no need to store any actual data; the below struct is
    // merely for reference and has no instance :)
    struct Fields
    {
        u64 sd: 1;      // shadowed
        u64 wpri_1: 29;
        u64 uxl: 2;     // shadowed
        u64 wpri_2: 12;
        u64 mxr: 1;     // shadowed
        u64 sum: 1;     // shadowed
        u64 wpri_3: 1;
        u64 xs: 2;      // shadowed
        u64 fs: 2;      // shadowed
        u64 wpri_4: 2;
        u64 vs: 2;      // shadowed
        u64 spp: 1;     // shadowed
        u64 wpri_5: 1;
        u64 ube: 1;     // shadowed
        u64 spie: 1;    // shadowed
        u64 wpri_6: 3;
        u64 sie: 1;     // shadowed
        u64 wpri_7: 1;
    };

    bool write(const u64 value, CPU& cpu) override;
    std::optional<u64> read(CPU& cpu) override;

    static_assert(sizeof(Fields) == sizeof(u64));
};

struct MISA : CSR
{
    bool write(const u64, CPU&) override;
    std::optional<u64> read(CPU& cpu) override;
};

struct MEDeleg : CSR
{
    u64 data = 0;

    bool write(const u64 value, CPU&) override
    {
        // WARL: medeleg[11] is read-only zero
        data = value & (~(1 << 11));
        return true;
    }

    std::optional<u64> read(CPU&) override
    {
        return data;
    }

    bool should_delegate(const u64 trap_id) const
    {
        return ((data >> (u64)trap_id) & 1) == 1;
    }
};

struct MIDeleg : DefaultCSR
{
    bool should_delegate(const u64 trap_id) const
    {
        return ((value >> (u64)trap_id) & 1) == 1;
    }
};

struct MIP : CSR
{
    // CSR is XLEN long but bits 16 and above designated for platform or custom use
    u16 bits = 0;

    MIP(u16 bits = 0) : bits(bits) {}

    bool write(const u64 value, CPU&) override
    {
        // WARL
        bits = value & 0b0000101010101010;
        return true;
    }

    std::optional<u64> read(CPU&) override
    {
        return bits;
    }

    bool mei() const { return ((bits >> 11) & 1) == 1; }
    bool sei() const { return ((bits >>  9) & 1) == 1; }
    bool uei() const { return ((bits >>  8) & 1) == 1; }
    bool mti() const { return ((bits >>  7) & 1) == 1; }
    bool sti() const { return ((bits >>  5) & 1) == 1; }
    bool msi() const { return ((bits >>  3) & 1) == 1; }
    bool ssi() const { return ((bits >>  1) & 1) == 1; }
    bool usi() const { return ((bits >>  0) & 1) == 1; }

    void set_mei()   { bits |=  (1 << 11); }
    void set_sei()   { bits |=  (1 << 9); }
    void set_uei()   { bits |=  (1 << 8); }
    void set_mti()   { bits |=  (1 << 7); }
    void set_sti()   { bits |=  (1 << 5); }
    void set_msi()   { bits |=  (1 << 3); }
    void set_ssi()   { bits |=  (1 << 1); }
    void set_usi()   { bits |=  (1 << 0); }

    void clear_mei() { bits &= ~(1 << 11); }
    void clear_sei() { bits &= ~(1 <<  9); }
    void clear_uei() { bits &= ~(1 <<  8); }
    void clear_mti() { bits &= ~(1 <<  7); }
    void clear_sti() { bits &= ~(1 <<  5); }
    void clear_msi() { bits &= ~(1 <<  3); }
    void clear_ssi() { bits &= ~(1 <<  1); }
    void clear_usi() { bits &= ~(1 <<  0); }
};

// Same fields as MIP, but meip = meie, seip = seie, etc.
struct MIE : MIP {};

// Shadows MIE
struct SIE : CSR
{
    bool write(const u64 value, CPU& cpu) override;
    std::optional<u64> read(CPU& cpu) override;
};

// Shadows MIP
struct SIP : CSR
{
    bool write(const u64 value, CPU& cpu) override;
    std::optional<u64> read(CPU& cpu) override;
};

struct SATP : CSR
{
    u64 bits = 0;

    enum class ModeSettings
    {
        None = 0,
        Sv39 = 8,
        Sv48 = 9,
        Sv57 = 10,
        Sv64 = 11
    };

    bool write(const u64 value, CPU& cpu) override
    {
        u64 old_bits = bits;
        bits = value;

        // "if satp is written with an unsupported MODE, the entire write has no
        // effect; no fields in satp are modified"
        if (get_mode() != ModeSettings::None && get_mode() != ModeSettings::Sv39)
            bits = old_bits;

        return true;
    }

    std::optional<u64> read(CPU& cpu) override
    {
        return bits;
    }

    ModeSettings get_mode() const
    {
        return ModeSettings((bits >> 60) & 0b1111);
    }

    // ASID = address space identifier
    u64 get_asid() const
    {
        return (bits >> 44) & 0b1111111111111111;
    }

    // PPN = physical page number
    u64 get_ppn() const
    {
        return bits & 0b11111111111111111111111111111111111111111111;
    }
};

struct Cycle : CSR
{
    bool write(const u64 value, CPU&) override;
    std::optional<u64> read(CPU&) override;
};

struct InstRet : Cycle
{
    std::optional<u64> read(CPU&) override;
};

struct UnimplementedCSR : CSR
{
    bool write(const u64 value, CPU& cpu) override;
    std::optional<u64> read(CPU&) override { return 0; }
};

// For values that are read-only and only ever return 0 - (eg marchid)
struct BlankCSR : CSR
{
    bool write(const u64 value, CPU&) override
    {
        return true;
    }

    std::optional<u64> read(CPU&) override
    {
        return 0;
    }
};
