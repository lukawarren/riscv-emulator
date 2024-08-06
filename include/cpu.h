#pragma once
#include "common.h"
#include "bus.h"
#include "sv39.h"
#include "csrs.h"
#include "traps.h"
#include "instruction.h"
#include "compressed_instruction.h"
#include "opcodes_f.h"

class CPU
{
public:
    CPU(
        const uint64_t ram_size,
        const bool emulating_test,
        const bool has_initramfs,
        const std::optional<std::string> block_device_image
    );
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

    /*
        Floating point registers and CSRs

        NOTE: Each register is a 64-bit wide double, but the floating point
              opcodes expect them to be 32-bit wide. To solve this, "float
              writes" to floating point registers are NaN-boxed - i.e. the
              upper 32 bits are set to 1 so that, when read as a double, the
              value becomes a NaN, and when read as a float the value is
              "normal".
    */
    double double_registers[32] = {};
    struct FloatRegisters
    {
        CPU* cpu;
        FloatRegisters() {}
        FloatRegisters(CPU* cpu) : cpu(cpu) {}

        void set(size_t index, float value)
        {
            // Perform NaN-boxing by setting all the "double bits" to 1
            // NaNs are canonicalised
            u32* u = (u32*)&value;
            u64 boxed = 0xffffffff00000000 | (u64)*u;
            memcpy(&cpu->double_registers[index], &boxed, sizeof(double));
        }

        float get(size_t index)
        {
            // Perform NaN-boxing by chopping off all the 1's from above
            // If it's not actually a NaN-boxed value, return the canonical NaN
            u64* u = (u64*)&cpu->double_registers[index];
            u32 new_bits;
            if((*u & 0xffffffff00000000) != 0xffffffff00000000)
                new_bits = qNaN_float;
            else
                new_bits = 0xffffffff & *u;
            float f;
            memcpy(&f, &new_bits, sizeof(float));
            return f;
        }

        float get_raw(size_t index)
        {
            // Floating point transfer operations don't do the NaN thing
            u64* u = (u64*)&cpu->double_registers[index];
            u32 new_bits = 0xffffffff & *u;
            float f;
            memcpy(&f, &new_bits, sizeof(float));
            return f;
        }

        struct Proxy
        {
            FloatRegisters& parent;
            size_t index;

            Proxy(FloatRegisters& parent, size_t index) : parent(parent), index(index) {}

            operator float() const
            {
                return parent.get(index);
            }

            Proxy& operator=(float value)
            {
                parent.set(index, value);
                return *this;
            }
        };

        Proxy operator[](size_t index)
        {
            return Proxy(*this, index);
        }
    };
    FloatRegisters float_registers;     // For NaN-boxing
    FCSR fcsr = {};                     // The "actual" CSR register
    FRM frm = {};                       // A "window" for the rounding mode
    FSFlags fsflags = {};               // A "window" for the fflags

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
    DefaultCSR time = {};               // Timer for RDTIME instruction
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
        bits |= (1 << 3);  // D
        bits |= (1 << 5);  // F
        bits |= (1 << 8);  // I
        bits |= (1 << 12); // M
        bits |= (1 << 18); // S (supervisor mode)
        bits |= (1 << 20); // U (user mode)
        return bits;
    }

    // RISC-V tests require the CPU to terminate when an ECALL occurs
    bool emulating_test = false;

    void invalidate_tlb();

private:
    void execute_instruction(const Instruction instruction);
    void execute_compressed_instruction(const CompressedInstruction instruction);
    u64 get_exception_cause(const Exception exception);

    struct TLBEntry
    {
        u64 virtual_page;
        u64 physical_page;
        u64 pte_address;
        u64 pte;
        AccessType access_type;
        bool valid = false;
    };
    std::array<TLBEntry, 4> tlb = {};
    size_t tlb_entries = 0;

    std::expected<u64, Exception> tlb_lookup(
        const u64 address,
        const AccessType type
    );

    void add_tlb_entry(
        const u64 virtual_page,
        const u64 physical_page,
        const PageTableEntry pte,
        const u64 pte_address,
        const AccessType access_type
    );

    void check_for_invalid_tlb();

    std::expected<u64, Exception> virtual_address_to_physical(
        const u64 address,
        const AccessType type
    );

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

    // For TLB
    PrivilegeLevel last_privilege_level = PrivilegeLevel::Machine;
    MStatus last_mstatus = {};

private:
    template<typename T>
    std::optional<T> fetch_from_bus(const u64 address)
    {
        if constexpr (sizeof(T) == 1) return bus.read_8(address);
        if constexpr (sizeof(T) == 2) return bus.read_16(address);
        if constexpr (sizeof(T) == 4) return bus.read_32(address);
        if constexpr (sizeof(T) == 8) return bus.read_64(address);
        return std::nullopt;
    }

    template<typename T>
    bool write_to_bus(const u64 address, const T value)
    {
        if constexpr (sizeof(T) == 1) return bus.write_8 (address, value);
        if constexpr (sizeof(T) == 2) return bus.write_16(address, value);
        if constexpr (sizeof(T) == 4) return bus.write_32(address, value);
        else                          return bus.write_64(address, value);
    }

    template<typename T>
    std::expected<T, Exception> read(const u64 address, const AccessType type)
    {
        if (paging_disabled(type))
        {
            const std::optional<T> value = fetch_from_bus<T>(address);
            if (!value)
            {
                if (type == AccessType::Instruction)
                    return std::unexpected(Exception::InstructionAccessFault);
                else
                    return std::unexpected(Exception::LoadAccessFault);
            }
            else return *value;
        }
        else
        {
            // Aligned access
            if ((address % sizeof(T)) == 0) [[likely]]
            {
                std::expected<u64, Exception> physical_address = tlb_lookup(address, type);
                if (physical_address.has_value()) [[likely]]
                {
                    const std::optional<T> value = fetch_from_bus<T>(*physical_address);
                    if (!value) [[unlikely]]
                    {
                        if (type == AccessType::Instruction)
                            return std::unexpected(Exception::InstructionAccessFault);
                        else
                            return std::unexpected(Exception::LoadAccessFault);
                    }
                    else
                        return *value;
                }
                else
                    return physical_address;
            }

            // Unaligned access
            else
            {
                T result = 0;
                for (size_t i = 0; i < sizeof(T); ++i)
                {
                    std::expected<u64, Exception> physical_address = tlb_lookup(address + i, type);
                    if (physical_address.has_value()) [[likely]]
                    {
                        const std::optional<u8> value = fetch_from_bus<u8>(*physical_address);
                        if (!value) [[unlikely]]
                        {
                            if (type == AccessType::Instruction)
                                return std::unexpected(Exception::InstructionAccessFault);
                            else
                                return std::unexpected(Exception::LoadAccessFault);
                        }
                        else
                            result |= (T(*value) << (i * 8));
                    }
                    else
                        return physical_address;
                }
                return result;
            }
        }
    }

    template<typename T>
    std::optional<Exception> write(const u64 address, const T value, const AccessType type)
    {
        if (paging_disabled(type))
        {
            if (!write_to_bus<T>(address, value))
                return Exception::StoreOrAMOAccessFault;
        }
        else
        {
            // Aligned access
            if ((address % sizeof(T)) == 0) [[likely]]
            {
                const std::expected<u64, Exception> virtual_address = tlb_lookup(address, type);
                if (virtual_address.has_value()) [[likely]]
                {
                    if (!write_to_bus<T>(*virtual_address, value)) [[unlikely]]
                        return Exception::StoreOrAMOAccessFault;
                }
                else
                    return virtual_address.error();
            }

            // Unaligned access
            else
            {
                for (size_t i = 0; i < sizeof(T); ++i)
                {
                    const std::expected<u64, Exception> virtual_address = tlb_lookup(address + i, type);
                    if (virtual_address.has_value()) [[likely]]
                    {
                        if (!write_to_bus<u8>(*virtual_address, (value >> (i * 8)) & 0xff)) [[unlikely]]
                            return Exception::StoreOrAMOAccessFault;
                    }
                    else
                        return virtual_address.error();
                }
            }
        }

        return std::nullopt;
    }
};
