#pragma once
#include "types.h"
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
#define CSR_MHARTID     0xf14

enum class PrivilegeLevel
{
    User,
    Supervisor,
    Hypervisor,
    Machine,
    Debug
};

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
        }
    }

    virtual void write(const u64 value) = 0;
    virtual u64 read() = 0;
};

struct MTVec: CSR
{
    u64 address;
    enum class Mode
    {
        Direct = 0,
        Vectored = 1
    } mode;

    void write(const u64 value) override
    {
        // TODO: check is aligned to 4-byte boundary
        address = value & 0xfffffffffffffffc;
        mode = (Mode)(value & 0b11);

        // WARL for mode; >=2 is reserved
        if ((u8)mode >= 2)
            mode = Mode::Direct;
    }

    u64 read() override
    {
        return address | (u64)mode;
    }
};

struct MEPC: CSR
{
    u64 address;

    void write(const u64 value) override
    {
        // WARL; lowest bit is always zero, and 2nd lowest is zero if IALIGN
        // can only be 32 (but we support 16, or will)
        address = value & 0xfffffffffffffffe;
    }

    u64 read() override
    {
        // Whenever IALIGN=32, bit mepc[1] is masked on reads so that it appears
        // to be 0. This masking occurs also for the implicit read by the MRET instruction.
        // Though masked, mepc[1] remains writable when IALIGN=32.
        std::cout << "TODO: respect IALIGN" << std::endl;
        return address;
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

    void write(const u64 value) override
    {
        memcpy(&fields, &value, sizeof(Fields));
        fields.wpri_1 = 0;
        fields.wpri_2 = 0;
        fields.wpri_3 = 0;
        fields.wpri_4 = 0;
        fields.wpri_5 = 0;
    }

    u64 read() override
    {
        u64 value;
        memcpy(&value, &fields, sizeof(Fields));
        return value;
    }

    static_assert(sizeof(Fields) == sizeof(u64));
};

struct MHartID : CSR
{
    void write(const u64 value) override {}

    u64 read() override
    {
        // Only one core for now! :)
        return 0;
    }
};

struct UnimplementedCSR : CSR
{
    void write(const u64 value) override
    {
        assert(value == 0);
    }

    u64 read() override { return 0; }
};
