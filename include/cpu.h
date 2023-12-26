#pragma once
#include "types.h"
#include "bus.h"
#include "csrs.h"
#include "traps.h"

class CPU
{
public:
    CPU(const uint64_t size, const bool emulating_test = false);
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
    SStatus sstatus = {};               // Status bits; effective shadow of mstatus
    DefaultCSR scounteren = {};         // Supervisor counter enable

    // Supervisor trap handilng
    SEPC sepc = {};

    // Supervisor Protection and Translation
    UnimplementedCSR satp;              // Supervisor address translation and protection

    // Machine trap setup
    MStatus mstatus = {};               // Status bits
    MISA misa = {};                     // ISA and extensions
    MEDeleg medeleg = {};               // Machine exception delegation register
    MIDeleg mideleg = {};               // Machine interrupt delegation register
    MIE mie = {};                       // Machine interrupt-enable register
    MTVec mtvec = {};                   // Machine trap-handler base address
    MCounterEnable mcounteren = {};     // Machine counter enable

    // Machine trap handling
    DefaultCSR mscratch = {};           // Scratch register for machine trap handlers
    MEPC mepc = {};                     // Machine exception program counter
    DefaultCSR mcause = {};             // Machine trap cause
    DefaultCSR mtval = {};              // Machine bad address or instruction
    MIP mip = {};                       // Machine interrupt pending
    UnimplementedCSR mtinst = {};       // Machine trap instruction (transformed)
    UnimplementedCSR mtval2 = {};       // Machine bad guest physical address

    // Machine counters / timers
    DefaultCSR mcycle = {};             // Cycle count
    DefaultCSR minstret = {};           // Counts instructions performed ("retired")

    // Debug registers
    DefaultCSR debug_registers[CSR_DEBUG_END - CSR_DEBUG_BEGIN + 1] = {};

    // Unprivileged counters / timers
    Cycle cycle = {};                   // Cycle counter; shadows mcycle
    InstRet instret = {};               // Shadows minstret

    // Machine information registers
    BlankCSR mvendorid = {};            // Vendor ID
    BlankCSR marchid = {};              // Arch ID
    BlankCSR mimpid = {};               // Implementation ID
    BlankCSR mhartid = {};              // ID of hart

    // Exceptions and interrupts
    void raise_exception(const Exception exception);
    void raise_exception(const Exception exception, const u64 info);
    void raise_interrupt(const Interrupt interrupt);
    std::optional<Interrupt> get_pending_interrupt();

    // Extensions
    static consteval u64 get_supported_extensions()
    {
        // 26 bits - one for each letter of alphabet, corresponding to exceptions
        // In addition, S and U represent support for supervisor and user mode.
        // The "I" bit is set for RV64I, etc., and "E" is set for RV64E, etc.
        u64 bits = 0;
        bits |= (1 << 0);  // A
        bits |= (1 << 8);  // E
        bits |= (1 << 12); // U
        bits |= (1 << 18); // S
        bits |= (1 << 20); // U
        return bits;
    }

    // RISC-V tests require the CPU to terminate when an ECALL occurs
    bool emulating_test = false;

    // Harts can suspend themselves when waiting for interrupts
    bool waiting_for_interrupts = false;

private:
    u64 get_exception_cause(const Exception exception);
    void handle_trap(const u64 cause, const u64 info, const bool interrupt);
    bool trap_did_occur = false;
};