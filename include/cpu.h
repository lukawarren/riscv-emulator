#pragma once
#include "types.h"
#include "bus.h"
#include "csrs.h"

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
    Bus bus;

    PrivilegeLevel privilege_level = PrivilegeLevel::Machine;

    // Supervisor Protection and Translation
    UnimplementedCSR satp;              // Supervisor address translation and protection

    // Machine information registers
    MHartID mhartid = {}; // ID of hart

    // Machine trap setup
    MStatus mstatus = {};               // Status bits
    UnimplementedCSR misa = {};         // ISA and extensions
    UnimplementedCSR medeleg = {};      // Machine exception delegation register
    UnimplementedCSR mideleg = {};      // Machine interrupt delegation register
    UnimplementedCSR mie = {};          // Machine interrupt-enable register
    MTVec mtvec = {};                   // Machine trap-handler base address
    UnimplementedCSR mcounteren = {};   // Machine counter enable

    // Machine trap handling
    UnimplementedCSR mscratch = {};     // Scratch register for machine trap handlers
    MEPC mepc = {};                     // Machine exception program counter
    UnimplementedCSR mcause = {};       // Machine trap cause
    UnimplementedCSR mtval = {};        // Machine bad address or instruction
    UnimplementedCSR mip = {};          // Machine interrupt pending
    UnimplementedCSR mtinst = {};       // Machine trap instruction (transformed)
    UnimplementedCSR mtval2 = {};       // Machine bad guest physical address

    // Exceptions
    enum class Exception
    {
        InstructionAddressMisaligned = 0,
        InstructionAccessFault = 1,
        IllegalInstruction = 2,
        Breakpoint = 3,
        LoadAddressMisaligned = 4,
        LoadAccessFault = 5,
        StoreOrAMOAddressMisaligned = 6,
        StoreOrAMOAccessFault = 7,
        EnvironmentCallFromUMode = 8,
        EnvironmentCallFromSMode = 9,
        EnvironmentCallFromMMode = 11,
        InstructionPageFault = 12,
        LoadPageFault = 13,
        StoreOrAMOPageFault = 15
    };
    void raise_exception(const Exception exception);
    bool exception_pending = false;
};