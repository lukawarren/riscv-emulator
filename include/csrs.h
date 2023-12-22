#pragma once
#include "types.h"
#include "exceptions.h"
#include <cstring>
#include <cassert>
#include <iostream>

#define CSR_SATP        0x180
#define CSR_MSTATUS     0x300
#define CSR_MISA        0x301
#define CSR_MEDELEG     0x302
#define CSR_MIDELEG     0x303
#define CSR_MIE         0x304
#define CSR_MTVEC       0x305
#define CSR_MCOUNTER_EN 0x306
#define CSR_MSCRATCH    0x340
#define CSR_MEPC        0x341
#define CSR_MCAUSE      0x342
#define CSR_MTVAL       0x343
#define CSR_MIP         0x344
#define CSR_MTINST      0x34a
#define CSR_MTVAL2      0x34b
#define CSR_PMPCFG0     0x3a0
#define CSR_PMPCFG15    0x3af
#define CSR_PMPADDR0    0x3b0
#define CSR_PMPADDR63   0x3ef
#define CSR_MNSTATUS    0x744
#define CSR_DEBUG_BEGIN 0x7a0
#define CSR_DEBUG_LIMIT 0x7af
#define CSR_DEBUG_END   0x7bf
#define CSR_MHARTID     0xf14

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

    virtual bool write(const u64 value, CPU& cpu) = 0;
    virtual u64 read(CPU& cpu) = 0;
};

struct MTVec : CSR
{
    u64 address;

    // WARL
    enum class Mode
    {
        Direct = 0,
        Vectored = 1
    } mode;

    bool write(const u64 value, CPU&) override
    {
        // Check is aligned to 4-byte boundary
        // TODO: check if needed when have compact instructions
        if ((value & 0x3) != 0)
            return false;

        address = value & 0xfffffffffffffffc;
        mode = (Mode)(value & 0b11);

        // WARL for mode; >=2 is reserved
        if ((u8)mode >= 2)
            mode = Mode::Direct;

        return true;
    }

    u64 read(CPU&) override
    {
        if (mode == Mode::Vectored) throw std::runtime_error("todo");
        return address | (u64)mode;
    }
};

struct MEPC : CSR
{
    u64 address;

    bool write(const u64 value, CPU&) override
    {
        // WARL; lowest bit is always zero, and 2nd lowest is zero if IALIGN
        // can only be 32 (but we support 16, or will)
        address = value & 0xfffffffffffffffe;
        return true;
    }

    u64 read(CPU&) override
    {
        // Whenever IALIGN=32, bit mepc[1] is masked on reads so that it appears
        // to be 0. This masking occurs also for the implicit read by the MRET instruction.
        // Though masked, mepc[1] remains writable when IALIGN=32.
        std::cout << "TODO: respect IALIGN" << std::endl;
        return address;
    }
};

struct MCause : CSR
{
    bool interrupt;
    u64 exception_code;

    bool write(const u64 value, CPU&) override
    {
        // Code is WLRL; not required to raise exception but could if we wanted to
        // Interrupt is set if trap was caused by an interrupt. TODO: emulate
        interrupt = false;
        exception_code = value;
        return true;
    }

    u64 read(CPU&) override
    {
        return exception_code & 0x7fffffffffffffff;
    }
};

struct MStatus : CSR
{
    struct Fields
    {
        u64 sd:     1;
        u64 wpri_1: 25;
        u64 mbe:    1;
        u64 sbe:    1;
        u64 sxl:    2;
        u64 uxl:    2;
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
    };
    Fields fields;

    MStatus()
    {
        memset(&fields, 0, sizeof(MStatus));
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
        return true;
    }

    u64 read(CPU&) override
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

struct MEDeleg : CSR
{
    u64 data;

    bool write(const u64 value, CPU&) override
    {
        data = value;
        return true;
    }

    u64 read(CPU&) override
    {
        return data;
    }

    bool should_delegate(const Exception type) const
    {
        return ((data >> (u64)type) & 1) == 1;
    }
};

struct MHartID : CSR
{
    bool write(const u64 value, CPU&) override { return true; }

    u64 read(CPU&) override {
        // Only one core for now! :)
        return 0;
    }
};

// No special restrictions or bits; just holds a value
struct DefaultCSR: CSR
{
    u64 value;

    bool write(const u64 value, CPU&) override
    {
        this->value = value;
        return true;
    }

    u64 read(CPU&) override {
        return value;
    }
};

struct UnimplementedCSR : CSR
{
    bool write(const u64 value, CPU&) override
    {
        assert(value == 0);
        return true;
    }

    u64 read(CPU&) override { return 0; }
};