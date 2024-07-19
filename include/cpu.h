#pragma once
#include "common.h"
#include "bus.h"
#include "csrs.h"
#include "traps.h"
#include "instruction.h"
#include "compressed_instruction.h"

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

    inline u64& sp() { return registers[2]; }

    PrivilegeLevel privilege_level = PrivilegeLevel::Machine;

    // Supervisor trap setup
    SStatus sstatus = {};               // Status bits; effective shadow of mstatus
    SIE sie = {};                       // Supervisor shadow of mie
    STVec stvec = {};                   // Supervisor trap-handler base address
    DefaultCSR scounteren = {};         // Supervisor counter enable

    // Supervisor trap handilng
    DefaultCSR sscratch = {};           // Scratch register for supervisor trap handlers
    SEPC sepc = {};                     // Supervisor exception program counter
    DefaultCSR scause = {};             // Supervisor trap cause
    DefaultCSR stval = {};              // Supervisor bad address or instruction
    SIP sip = {};                       // Supervisor shadow of mip

    // Supervisor Protection and Translation
    SATP satp = {};                     // Supervisor address translation and protection

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

    // Can't just deal with exceptions as soon as they occur due to priority issues
    // Have to delay it and keep track of state first
    struct PendingTrap
    {
        u64 cause;
        u64 info;
        bool is_interrupt;
    };
    std::optional<PendingTrap> pending_trap = {};

    // Exceptions and interrupts
    void raise_exception(const Exception exception);
    void raise_exception(const Exception exception, const u64 info);
    std::optional<PendingTrap> get_pending_trap();
    void handle_trap(const u64 cause, const u64 info, const bool interrupt);

    enum class AccessType
    {
        Instruction,
        Load,
        Store,
        Trace // For internal program use
    };

    [[nodiscard]] std::expected<u8,  Exception> read_8 (const u64 address, const AccessType type = AccessType::Load);
    [[nodiscard]] std::expected<u16, Exception> read_16(const u64 address, const AccessType type = AccessType::Load);
    [[nodiscard]] std::expected<u32, Exception> read_32(const u64 address, const AccessType type = AccessType::Load);
    [[nodiscard]] std::expected<u64, Exception> read_64(const u64 address, const AccessType type = AccessType::Load);

    [[nodiscard]] std::optional<Exception>      write_8 (const u64 address, const u8  value, const AccessType type = AccessType::Store);
    [[nodiscard]] std::optional<Exception>      write_16(const u64 address, const u16 value, const AccessType type = AccessType::Store);
    [[nodiscard]] std::optional<Exception>      write_32(const u64 address, const u32 value, const AccessType type = AccessType::Store);
    [[nodiscard]] std::optional<Exception>      write_64(const u64 address, const u64 value, const AccessType type = AccessType::Store);

    // Extensions
    static consteval u64 get_supported_extensions()
    {
        // 26 bits - one for each letter of alphabet, corresponding to exceptions
        // In addition, S and U represent support for supervisor and user mode.
        // The "I" bit is set for RV64I, etc., and "E" is set for RV64E, etc.
        u64 bits = 0;
        bits |= (1 << 0);  // A
        bits |= (1 << 2);  // C
        bits |= (1 << 8);  // E
        bits |= (1 << 12); // U
        bits |= (1 << 18); // S
        bits |= (1 << 20); // U
        return bits;
    }

    // RISC-V tests require the CPU to terminate when an ECALL occurs
    bool emulating_test = false;

private:
    void execute_instruction(const Instruction instruction);
    void execute_compressed_instruction(const CompressedInstruction instruction);
    u64 get_exception_cause(const Exception exception);

    std::expected<u64, Exception> virtual_address_to_physical(
        const u64 address,
        const AccessType type
    );

    template<typename T>
    inline std::expected<T, Exception> read_bytes(const u64 address, const AccessType type)
    {
        T value = 0;

        for (size_t i = 0; i < sizeof(T); ++i)
        {
            const auto a = read_8(address + i, type);
            if (!a.has_value()) return std::unexpected(a.error());
            value |= (T(*a) << (i * 8));
        }

        return value;
    }

    template<typename T>
    inline std::optional<Exception> write_bytes(const u64 address, T value, const AccessType type)
    {
        for (size_t i = 0; i < sizeof(T); ++i)
        {
            const auto a = write_8(address + i, (value >> (i * 8)) & 0xff, type);
            if (a.has_value()) return a;
        }

        return std::nullopt;
    }

    inline PrivilegeLevel effective_privilege_level(const AccessType type) const
    {
        // When MPRV=1, load and store memory addresses are translated and
        // protected, and endianness is applied, as though the current privilege
        // mode were set to MPP
        if ((type == AccessType::Load || type == AccessType::Store) && mstatus.fields.mprv == 1)
            return (PrivilegeLevel)mstatus.fields.mpp;
        else
            return privilege_level;
    }

    inline bool paging_disabled(AccessType type) const
    {
        // TODO: cache
        if (type == AccessType::Load || type == AccessType::Store)
        {
            return satp.get_mode() == SATP::ModeSettings::None ||
                effective_privilege_level(type) == PrivilegeLevel::Machine;
        }
        else
        {
            return satp.get_mode() == SATP::ModeSettings::None ||
                privilege_level == PrivilegeLevel::Machine;
        }
    }

    // For mcause
    u64 erroneous_virtual_address;
};
