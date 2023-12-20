#pragma once
#include "types.h"
#include "bus.h"

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
 */
struct CSR
{
    u64 data;

    CSR(const u64 data) : data(data) {}

    bool is_read_only(const u16 address) const
    {
        return ((address >> 10) & 0b11) == 0b11;
    }

    PrivilegeLevel get_privilege_level(const u16 address) const
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
};

class CPU
{
public:
    CPU(const uint64_t size);
    void cycle();
    void trace();

    /*
        32 integer registers:
        - x0 (zero):        always zero
        - x1 (ra):          return address
        - x2 (sp):          stack pointer
        - x3 (gp):          global pointer
        - x4 (tp):          thread pointer
        - x5 (t0):          temporary return address
        - x6-7 (t1-2):      temporaries
        - x8 (s0/fp):       saved register / frame pointer
        - x9 (s1):          saved register
        - x10-11 (a0-1):    function arguments / return values
        - x12-17 (a2-7):    function arguments
        - x18-27 (s2-11):   saved registers
        - x28-31 (t3-6):    temporaries
    */
    u64 registers[32] = {};
    u64 pc = 0;

    // CSRs
    CSR mtvec = { 0 };
    CSR mepec = { 0 }; // virtual address of instruction interrupted before exception occured
    CSR mnstatus = { 0 }; // Smrnmi extension; only for tests
    CSR hartid = { 0 };

    Bus bus;
};