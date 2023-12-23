#pragma once
#include "types.h"
#include "bus.h"
#include "csrs.h"
#include "exceptions.h"

class CPU
{
public:
    CPU(const uint64_t size);
    void do_cycle();
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
    Bus bus;

    PrivilegeLevel privilege_level = PrivilegeLevel::Machine;

    // Supervisor trap setup
    DefaultCSR scounteren = {};         // Supervisor counter enable

    // Supervisor Protection and Translation
    UnimplementedCSR satp;              // Supervisor address translation and protection

    // Machine information registers
    MHartID mhartid = {}; // ID of hart

    // Machine trap setup
    MStatus mstatus = {};               // Status bits
    MISA misa = {};                     // ISA and extensions
    MEDeleg medeleg = {};               // Machine exception delegation register
    UnimplementedCSR mideleg = {};      // Machine interrupt delegation register
    UnimplementedCSR mie = {};          // Machine interrupt-enable register
    MTVec mtvec = {};                   // Machine trap-handler base address
    MCounterEnable mcounteren = {};     // Machine counter enable

    // Machine trap handling
    DefaultCSR mscratch = {};           // Scratch register for machine trap handlers
    MEPC mepc = {};                     // Machine exception program counter
    MCause mcause = {};                 // Machine trap cause
    DefaultCSR mtval = {};              // Machine bad address or instruction
    UnimplementedCSR mip = {};          // Machine interrupt pending
    UnimplementedCSR mtinst = {};       // Machine trap instruction (transformed)
    UnimplementedCSR mtval2 = {};       // Machine bad guest physical address

    // Machine counters / timers
    DefaultCSR mcycle = {};             // Cycle count

    // Debug registers
    DefaultCSR debug_registers[CSR_DEBUG_END - CSR_DEBUG_BEGIN + 1] = {};

    // Unprivileged counters / timers
    Cycle cycle = {};                   // Cycle counter; shadows mcycle

    // Exceptions
    void raise_exception(const Exception exception, const u64 info = 0);
    bool exception_did_occur = false;

    // Extensions
    static consteval u64 get_supported_extensions()
    {
        // 26 bits - one for each letter of alphabet, corresponding to exceptions
        // In addition, S and U represent support for supervisor and user mode.
        // The "I" bit is set for RV64I, etc., and "E" is set for RV64E, etc.
        u64 bits = 0;
        bits |= (1 << 8);  // E
        bits |= (1 << 18); // S
        bits |= (1 << 20); // U
        return bits;
    }
};