#include "cpu.h"
#include "compressed_instruction.h"
#include "opcodes_base.h"
#include "opcodes_zicsr.h"
#include "opcodes_m.h"
#include "opcodes_a.h"
#include "opcodes_c.h"
#include "opcodes_f.h"
#include "traps.h"
#include "dtb.h"

extern "C" {
    #include <riscv-disas.h>
}

CPU::CPU(
    const uint64_t ram_size,
    const bool emulating_test,
    const std::optional<std::string> block_device_image
) :
    bus(ram_size, block_device_image, emulating_test), emulating_test(emulating_test)
{
    // Set x0 to 0, sp to end of memory and pc to start of RAM
    registers[0] = 0;
    registers[2] = Bus::ram_base + ram_size;
    pc = Bus::programs_base;

    // Work out DTB address - aligned to nearest page
    u64 dtb_address = Bus::ram_base + ram_size - sizeof(DTB) - 1;
    dtb_address = dtb_address / 4096 * 4096;

    // Load DTB into memory
    for (size_t i = 0; i < sizeof(DTB); ++i)
        std::ignore = write_8(dtb_address + i, DTB[i]);

    // Set x11 to DTB pointer and x10 to hart id
    registers[10] = 0;
    registers[11] = dtb_address;

    // Init floats
    float_registers = FloatRegisters(this);
    init_opcodes_f();
}

void CPU::do_cycle()
{
    check_for_invalid_tlb();

    // Check is 16-bit aligned (32 if RVC weren't supported)
    if ((pc & 0b1) != 0)
    {
        raise_exception(Exception::InstructionAddressMisaligned, pc);
        return;
    }

    // Check if instruction is of compressed form
    const std::expected<CompressedInstruction, Exception> half_instruction = read_16(pc, AccessType::Instruction);
    const bool is_compressed = (half_instruction.has_value() && (half_instruction->instruction & 0b11) != 0b11);

    // Else fetch regular instruction
    std::expected<Instruction, Exception> instruction = std::unexpected(Exception::IllegalInstruction);
    if (!is_compressed) instruction = read_32(pc, AccessType::Instruction);
    if (!is_compressed && !instruction)
    {
        // Exception information needs the vaddr of the portion of the
        // instruction that caused the fault
        u64 faulty_address = pc;
        if (!read_8(pc)) faulty_address = pc;
        else if (!read_8(pc + 1)) faulty_address = pc + 1;
        else if (!read_8(pc + 1)) faulty_address = pc + 2;
        else faulty_address = pc + 3;

        raise_exception(instruction.error(), faulty_address);
        return;
    }

    // Check it's valid
    if (!is_compressed &&
        (instruction->instruction == 0xffffffff ||
        instruction->instruction == 0))
    {
        raise_exception(Exception::IllegalInstruction, instruction->instruction);
        return;
    }
    else if (is_compressed && half_instruction->instruction == 0x0000)
    {
        raise_exception(Exception::IllegalInstruction, half_instruction->instruction);
        return;
    }

    // Reset x0
    registers[0] = 0;

    // Seems to be a valid instruction - try and execute it
    if (is_compressed) execute_compressed_instruction(*half_instruction);
    else execute_instruction(*instruction);

    mcycle.increment(*this);
    minstret.increment(*this);
    time.increment(*this);
}

void CPU::trace()
{
    // Get next instruction
    std::expected<Instruction, Exception> instruction = std::unexpected(Exception::IllegalInstruction);
    const std::expected<CompressedInstruction, Exception> half_instruction = read_16(pc, AccessType::Trace);
    const bool is_compressed = (half_instruction.has_value() && (half_instruction->instruction & 0b11) != 0b11);
    if (!is_compressed) instruction = read_32(pc, AccessType::Trace);

    if (instruction.has_value() || half_instruction.has_value())
    {
        char buf[80] = { 0 };
        disasm_inst(
            buf,
            sizeof(buf),
            rv64,
            pc,
            instruction.has_value() ? instruction->instruction : half_instruction->instruction
        );
        printf("%016" PRIx64 ":  %s\n", pc, buf);
    }
    else std::cout << "??" << std::endl;
}

void CPU::raise_exception(const Exception exception)
{
    raise_exception(exception, get_exception_cause(exception));
}

void CPU::raise_exception(const Exception exception, const u64 info)
{
    if (exception != Exception::EnvironmentCallFromUMode &&
        exception != Exception::EnvironmentCallFromSMode &&
        exception != Exception::EnvironmentCallFromMMode &&
        emulating_test)
        dbg("warning: exception occurred", (int)exception, dbg::hex(pc), dbg::hex(info));

    assert(!pending_trap.has_value());
    pending_trap = PendingTrap { (u64)exception, info, false };
}

std::optional<CPU::PendingTrap> CPU::get_pending_trap()
{
    // Deal with exceptions caused this CPU cycle first to avoid the issue of
    // timer interrupts (for example) and ecall's happening at the same time
    // and causing all sorts of strange bugs. Instead just deal with traps
    // first. This is not exactly accurate to the spec.
    if (pending_trap.has_value())
    {
        const auto trap = *pending_trap;
        pending_trap.reset();
        return trap;
    }

    // Check interrupts are enabled before we return any
    if ((privilege_level == PrivilegeLevel::Machine && mstatus.fields.mie == 0) ||
         (privilege_level == PrivilegeLevel::Supervisor && mstatus.fields.sie == 0))
        return std::nullopt;

    // Use bitmask to find all interrupts that are both pending and enabled
    // For each possible source, raise interrupt if found, and clear pending bit
    const MIP pending(mie.bits & mip.bits);
    if (pending.mei()) { mip.clear_mei(); return PendingTrap { (u64)Interrupt::MachineExternal,     0, true }; }
    if (pending.msi()) { mip.clear_msi(); return PendingTrap { (u64)Interrupt::MachineSoftware,     0, true }; }
    if (pending.mti()) { mip.clear_mti(); return PendingTrap { (u64)Interrupt::MachineTimer,        0, true }; }
    if (pending.sei()) { mip.clear_sei(); return PendingTrap { (u64)Interrupt::SupervisorExternal,  0, true }; }
    if (pending.ssi()) { mip.clear_ssi(); return PendingTrap { (u64)Interrupt::SupervisorSoftware,  0, true }; }
    if (pending.sti()) { mip.clear_sti(); return PendingTrap { (u64)Interrupt::SupervisorTimer,     0, true }; }

    return std::nullopt;
}

void CPU::handle_trap(const u64 cause, const u64 info, const bool interrupt)
{
    /*
        By default, all traps at any privilege level are handled in machine mode,
        though a machine-mode handler can redirect traps back to the appropriate
        level with the MRET instruction. To increase performance, implementations
        can provide individual read/write bits within medeleg and mideleg to indicate
        that certain exceptions and interrupts should be processed directly by a
        lower privilege level.
     */

    const u64 original_pc = pc;
    const PrivilegeLevel original_privilege_level = privilege_level;

    const bool should_delegate = interrupt ?
        mideleg.should_delegate(cause):
        medeleg.should_delegate(cause);

    if (privilege_level <= PrivilegeLevel::Supervisor && should_delegate)
    {
        // Handle in supervisor mode
        privilege_level = PrivilegeLevel::Supervisor;
        if (stvec.mode == STVec::Mode::Vectored)
            pc = stvec.address + cause * 4;
        else
            pc = stvec.address;

        // As below but in supervisor mode
        sepc.write(original_pc & (~1), *this);
        scause.write((u64)cause | ((u64)interrupt << 63), *this);
        stval.write(info, *this);
        mstatus.fields.spie = mstatus.fields.sie;
        mstatus.fields.sie = 0;

        // Different from below
        mstatus.fields.spp = (original_privilege_level == PrivilegeLevel::User) ? 0 : 1;
    }
    else
    {
        // Handle trap in machine mode
        privilege_level = PrivilegeLevel::Machine;
        if(mtvec.mode == MTVec::Mode::Vectored)
            pc = mtvec.address + cause * 4;
        else
            pc = mtvec.address;

        // Set mepc to virtual address of instruction that was interrupted
        // The lower bit must be zero
        mepc.write(original_pc & (~1), *this);

        // Set mcause to cause - interrupts have MSB set
        mcause.write((u64)cause | ((u64)interrupt << 63), *this);

        // Set mtval to (optional) exception-specific information
        mtval.write(info, *this);

        // Set PIE bit in mstatus to MIE bit ("IE" = interrupt enable)
        mstatus.fields.mpie = mstatus.fields.mie;

        // Disable interrupts
        mstatus.fields.mie = 0;

        // Record previous privilege
        mstatus.fields.mpp = (u64)original_privilege_level;
    }
}

u64 CPU::get_exception_cause(const Exception exception)
{
    switch (exception)
    {
        case Exception::IllegalInstruction:
            return *read_32(pc, AccessType::Trace);

        case Exception::LoadAccessFault:
            return pc;

        case Exception::StoreOrAMOAccessFault:
            return pc;

        case Exception::StoreOrAMOPageFault:
        case Exception::LoadPageFault:
        case Exception::InstructionPageFault:
            return erroneous_virtual_address;

        case Exception::EnvironmentCallFromUMode:
        case Exception::EnvironmentCallFromSMode:
        case Exception::EnvironmentCallFromMMode:
            return 0;

        default:
            throw std::runtime_error(
                "unable to determine exception casue - "
                "wrong overload used or TODO"
            );
    };
}

void CPU::execute_instruction(const Instruction instruction)
{
    const u8 opcode = instruction.get_opcode();
    const u8 funct3 = instruction.get_funct3();
    const u8 funct7 = instruction.get_funct7();

    // Decode - try base cases first because ECALL and ZICSR overlap
    bool did_find_opcode = opcodes_base(*this, instruction);
    if (!did_find_opcode)
    {
        switch(opcode)
        {
            case OPCODES_ZICSR:
                did_find_opcode = opcodes_zicsr(*this, instruction);
                break;

            case OPCODES_M:
            case OPCODES_M_32:
                // Distinguished from OPCODES_BASE_R_TYPE[_32] by funct7
                if (funct7 == OPCODES_M_FUNCT_7)
                    did_find_opcode = opcodes_m(*this, instruction);
                break;

            case OPCODES_A:
                did_find_opcode = opcodes_a(*this, instruction);
                break;

            case OPCODES_F_1:
            case OPCODES_F_2:
            case OPCODES_F_3:
            case OPCODES_F_4:
            case OPCODES_F_5:
            case OPCODES_F_6:
            case OPCODES_F_7:
                did_find_opcode = opcodes_f(*this, instruction);
                break;

            default:
                break;
        }
    }

    if (!did_find_opcode)
        throw std::runtime_error(std::format(
            "unknown opcode 0x{:x} with funct3 0x{:x}, funct7 0x{:x}, rs2 0x{:x} - raw = 0x{:x}, pc = 0x{:x}",
            opcode,
            funct3,
            funct7,
            instruction.get_rs2(),
            instruction.instruction,
            pc
        ));

    if (!pending_trap.has_value())
        pc += sizeof(u32);
}

void CPU::execute_compressed_instruction(const CompressedInstruction instruction)
{
    if (!opcodes_c(*this, instruction))
        throw std::runtime_error(std::format(
            "unknown opcode 0x{:x} with funct3 0x{:x} - raw = 0x{:x}, pc = 0x{:x}",
            instruction.get_opcode(),
            instruction.get_funct3(),
            instruction.instruction,
            pc
        ));

    if (!pending_trap.has_value())
        pc += sizeof(u16);
}

void CPU::invalidate_tlb()
{
    tlb_entries = 0;
}

std::expected<u64, Exception> CPU::tlb_lookup(
    const u64 address,
    const AccessType type
)
{
    const u64 page_size = 4096;
    const u64 virtual_page = address / page_size;

    // Check TLB first
    for (size_t i = 0; i < tlb_entries; ++i)
    {
        const auto& entry = tlb[i];
        if (entry.valid && entry.virtual_page == virtual_page && entry.access_type == type)
        {
            // Before we return the page, we must bear in mind that we are technically
            // "accessing" it. Hence, the A and D bits must be dealt with accordingly.
            // (see step 7 of the MMU)
            PageTableEntry pte(entry.pte);
            if (pte.get_a() == 0 || (type == AccessType::Store && pte.get_d() == 0))
            {
                if (type != AccessType::Trace)
                {
                    pte.set_a();
                    if (type == AccessType::Store)
                        pte.set_d();

                    // TODO: PMA or PMP check
                    if (emulating_test)
                        dbg("TODO: pma or pmp MMU check missed");

                    // Update PTE value
                    std::ignore = bus.write_64(entry.pte_address, pte.address);
                }
            }

            return entry.physical_page * page_size + (address % page_size);
        }
    }

    // Failed; perform proper lookup
    const auto result = virtual_address_to_physical(address, type);
    return result;
}

void CPU::add_tlb_entry(
    const u64 virtual_page,
    const u64 physical_page,
    const PageTableEntry pte,
    const u64 pte_address,
    const AccessType type
)
{
    // Evict oldest element if need be
    if (tlb_entries == tlb.size())
    {
        for (size_t i = 0; i < tlb.size() - 1; ++i)
            tlb[i] = tlb[i + 1];

        tlb_entries--;
    }

    tlb[tlb_entries].physical_page = physical_page;
    tlb[tlb_entries].virtual_page = virtual_page;
    tlb[tlb_entries].pte = pte.address;
    tlb[tlb_entries].pte_address = pte_address;
    tlb[tlb_entries].access_type = type;
    tlb[tlb_entries].valid = true;
    tlb_entries++;
}

void CPU::check_for_invalid_tlb()
{
    // Certain staes modify page "permissions", like mstatus or the current privilege
    // level. When these change, we can no longer assume that any previously cached
    // translation is valid.

    // NOTE: modifications to SATP is handled separately

    // TODO: avoid branch by just putting this in csrs.cpp

    if (mstatus.fields.mxr != last_mstatus.fields.mxr ||
        mstatus.fields.sum != last_mstatus.fields.sum ||
        mstatus.fields.mprv != last_mstatus.fields.mprv ||
        mstatus.fields.mpp != last_mstatus.fields.mpp ||
        last_privilege_level != privilege_level)
        invalidate_tlb();

    last_privilege_level = privilege_level;
    last_mstatus = mstatus;
}

// Implements Sv39 paging - see RISC-V Instruction Set Manual Volume II - Privileged Architecture
std::expected<u64, Exception> CPU::virtual_address_to_physical(
    const u64 address,
    const AccessType type
)
{
    const u64 page_size = 4096;
    const u64 levels = 3;
    const u64 pte_size = 8;

    const VirtualAddress va(address);
    const auto vpns = va.get_vpns();

    PageTableEntry pte(0);
    u64 pte_address;

    const auto appropriate_exception = [&](int number)
    {
        if (type != AccessType::Trace && emulating_test)
            dbg(
                "MMU exception",
                dbg::hex(address),
                number,
                type,
                privilege_level,
                effective_privilege_level(type),
                (int)mstatus.fields.mpp,
                (int)mstatus.fields.mprv,
                dbg::hex(pc)
            );

        // Store cause
        erroneous_virtual_address = address;
        switch (type)
        {
            case AccessType::Instruction: return std::unexpected(Exception::InstructionPageFault);
            case AccessType::Load:        return std::unexpected(Exception::LoadPageFault);
            case AccessType::Store:       return std::unexpected(Exception::StoreOrAMOPageFault);
            case AccessType::Trace:       return std::unexpected(Exception::InternalProgramUse);
            default: assert(false);
        }
    };

    // 1. Let a be satp.ppn × PAGESIZE, and let i = LEVELS − 1.
    u64 a = satp.get_ppn() * page_size;
    i64 i = levels - 1;

    while(true)
    {
        // 2. Let pte be the value of the PTE at address a+va.vpn[i]×PTESIZE.
        //    If accessing pte violates a PMA or PMP check, raise an access exception.
        // TODO: PMA and PMP checks
        pte_address = a + vpns[i] * pte_size;
        const auto pte_opt = bus.read_64(pte_address);
        assert(pte_opt.has_value());
        pte = PageTableEntry(*pte_opt);

        // 3. If pte.v = 0, or if pte.r = 0 and pte.w = 1, stop and raise a page-fault exception.
        if (pte.get_v() == 0 || (pte.get_r() == 0 && pte.get_w() == 1))
            return appropriate_exception(1);

        // 4. Otherwise, the PTE is valid. If pte.r = 1 or pte.x = 1, go to step 5.
        //    Otherwise, this PTE is a pointer to the next level of the page table.
        //    Let i = i − 1. If i < 0, stop and raise a page-fault exception. Otherwise,
        //    let a = pte.ppn × PAGESIZE and go to step 2.

        if (pte.get_r() == 1 || pte.get_x() == 1)
            break;

        i -= 1;
        if (i < 0)
            return appropriate_exception(2);

        a = pte.get_ppn() * page_size;
    }

    // 5. A leaf PTE has been found. Determine if the requested memory access is
    //    allowed by the pte.r, pte.w, pte.x, and pte.u bits, given the current
    //    privilege mode and the value of the SUM and MXR fields of the mstatus
    //    register. If not, stop and raise a page-fault exception.
    //
    //    3.1.6.3 Memory Privilege in mstatus Register
    //    "The MXR (Make eXecutable Readable) bit modifies the privilege with which loads access
    //    virtual memory. When MXR=0, only loads from pages marked readable (R=1 in Figure 4.15)
    //    will succeed. When MXR=1, loads from pages marked either readable or executable
    //    (R=1 or X=1) will succeed. MXR has no effect when page-based virtual memory is not in
    //    effect. MXR is hardwired to 0 if S-mode is not supported."
    //
    //    "The SUM (permit Supervisor User Memory access) bit modifies the privilege with which
    //    S-mode loads and stores access virtual memory. When SUM=0, S-mode memory accesses to
    //    pages that are accessible by U-mode (U=1 in Figure 4.15) will fault. When SUM=1, these
    //    accesses are permitted.  SUM has no effect when page-based virtual memory is not in
    //    effect. Note that, while SUM is ordinarily ignored when not executing in S-mode, it is
    //    in effect when MPRV=1 and MPP=S. SUM is hardwired to 0 if S-mode is not supported."

    // MXR bit
    if (type == AccessType::Load)
    {
        if (mstatus.fields.mxr == 0 && pte.get_r() != 1)
            return appropriate_exception(3);

        if (mstatus.fields.mxr == 1 && (pte.get_r() != 1 && pte.get_x() != 1))
            return appropriate_exception(4);
    }

    if (type != AccessType::Trace)
    {
        // SUM bit
        const PrivilegeLevel privilege = effective_privilege_level(type);
        if (mstatus.fields.sum == 0 && privilege == PrivilegeLevel::Supervisor && pte.get_u() == 1)
            return appropriate_exception(5);

        // pte.w
        if (pte.get_w() != 1 && type == AccessType::Store)
            return appropriate_exception(6);

        // pte.x
        if (pte.get_x() != 1 && type == AccessType::Instruction)
            return appropriate_exception(7);

        // pte.u
        if (pte.get_u() != 1 && privilege == PrivilegeLevel::User)
            return appropriate_exception(8);
    }


    // 6. If i > 0 and pte.ppn[i − 1 : 0] != 0, this is a misaligned superpage;
    //    stop and raise a page-fault exception.
    if (i > 0)
    {
        for (int j = i - 1; j >= 0; --j)
            if (pte.get_ppns()[j] != 0)
                return appropriate_exception(9);
    }

    // 7. If pte.a = 0, or if the memory access is a store and pte.d = 0, either raise
    //    a page-fault exception or:
    //    - Set pte.a to 1 and, if the memory access is a store, also set pte.d to 1
    //    - If this access violates a PMA or PMP check, raise an access exception.
    //    - This update and the loading of pte in step 2 must be atomic; in particular,
    //      no intervening store to the PTE may be perceived to have occurred in-between.
    if (pte.get_a() == 0 || (type == AccessType::Store && pte.get_d() == 0))
    {
        if (type != AccessType::Trace)
        {
            pte.set_a();
            if (type == AccessType::Store)
                pte.set_d();

            // TODO: PMA or PMP check
            if (emulating_test)
                dbg("TODO: pma or pmp MMU check missed");

            // Update PTE value
            std::ignore = bus.write_64(a + vpns[i] * pte_size, pte.address);
        }
    }

    // 8. The translation is successful. The translated physical address is given as follows:
    //    pa.pgoff = va.pgoff
    //    If i > 0, then this is a superpage translation and pa.ppn[i−1:0] = va.vpn[i−1:0].
    //    pa.ppn[LEVELS−1:i] = pte.ppn[LEVELS−1:i].
    const u64 offset = va.get_page_offset();
    switch(i)
    {
        case 0:
        {
            u64 ppn = pte.get_ppn();
            u64 addr = (ppn << 12) | offset;
            add_tlb_entry(address / page_size, addr / page_size, pte, pte_address, type);
            return addr;
        }
        case 1:
        {
            const auto ppns = pte.get_ppns();
            u64 addr = (ppns[2] << 30) | (ppns[1] << 21) | (vpns[0] << 12) | offset;
            add_tlb_entry(address / page_size, addr / page_size, pte, pte_address, type);
            return addr;
        }
        case 2:
        {
            const auto ppns = pte.get_ppns();
            u64 addr = (ppns[2] << 30) | (vpns[1] << 21) | (vpns[0] << 12) | offset;
            add_tlb_entry(address / page_size, addr / page_size, pte, pte_address, type);
            return addr;
        }
        default:
            return appropriate_exception(10);
    }
}

std::expected<u8,  Exception> CPU::read_8 (const u64 address, const AccessType type) { return read<u8> (address, type); }
std::expected<u16, Exception> CPU::read_16(const u64 address, const AccessType type) { return read<u16>(address, type); }
std::expected<u32, Exception> CPU::read_32(const u64 address, const AccessType type) { return read<u32>(address, type); }
std::expected<u64, Exception> CPU::read_64(const u64 address, const AccessType type) { return read<u64>(address, type); }

std::optional<Exception> CPU::write_8 (const u64 address, const u8  value, const AccessType type) { return write<u8>  (address, value, type); }
std::optional<Exception> CPU::write_16(const u64 address, const u16 value, const AccessType type) { return write<u16> (address, value, type); }
std::optional<Exception> CPU::write_32(const u64 address, const u32 value, const AccessType type) { return write<u32> (address, value, type); }
std::optional<Exception> CPU::write_64(const u64 address, const u64 value, const AccessType type) { return write<u64> (address, value, type); }
