#include "cpu.h"
#include "compressed_instruction.h"
#include "opcodes_base.h"
#include "opcodes_zicsr.h"
#include "opcodes_m.h"
#include "opcodes_a.h"
#include "opcodes_c.h"
#include "dtb.h"
#include "traps.h"
#include <iostream>
#include <format>

extern "C" {
    #include <riscv-disas.h>
}

CPU::CPU(const u64 ram_size, const bool emulating_test) :
    bus(ram_size, emulating_test) , emulating_test(emulating_test)
{
    // Set x0 to 0, sp to end of memory and pc to start of RAM
    registers[0] = 0;
    registers[2] = Bus::ram_base + ram_size;
    pc = Bus::programs_base;

    // Load DTB into memory
    const u64 dtb_address = Bus::ram_base + ram_size - sizeof(DTB);
    for (size_t i = 0; i < sizeof(DTB); ++i)
        std::ignore = bus.write_8(dtb_address + i, DTB[i]);

    // Set x11 to DTB pointer and x10 to hart id
    registers[10] = 0;
    registers[11] = dtb_address;
}

void CPU::do_cycle()
{
    // Check is 16-bit aligned (32 if RVC weren't supported)
    if ((pc & 0b1) != 0)
    {
        raise_exception(Exception::InstructionAddressMisaligned, pc);
        return;
    }

    // Check if instruction is of compressed form
    const std::optional<CompressedInstruction> half_instruction = { bus.read_16(pc) };
    const bool is_compressed = (half_instruction.has_value() && (half_instruction->instruction & 0b11) != 0b11);

    // Else fetch regular instruction
    std::optional<Instruction> instruction;
    if (!is_compressed) instruction = bus.read_32(pc);
    if (!is_compressed && !instruction)
    {
        // Exception information needs the vaddr of the portion of the
        // instruction that caused the fault
        u64 faulty_address = pc;
        if (!bus.read_8(pc)) faulty_address = pc;
        else if (!bus.read_8(pc + 1)) faulty_address = pc + 1;
        else if (!bus.read_8(pc + 1)) faulty_address = pc + 2;
        else faulty_address = pc + 3;

        raise_exception(Exception::InstructionAccessFault, faulty_address);
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
        raise_exception(Exception::IllegalInstruction, instruction->instruction);
        return;
    }

    // Reset x0
    registers[0] = 0;

    // Seems to be a valid instruction - try and execute it
    if (is_compressed) execute_compressed_instruction(*half_instruction);
    else execute_instruction(*instruction);

    mcycle.increment(*this);
    minstret.increment(*this);
}

void CPU::trace()
{
    // Get next instruction
    // TODO: guard against modifying state via extraneous reads
    std::optional<Instruction> instruction;
    const std::optional<CompressedInstruction> half_instruction = { bus.read_16(pc) };
    const bool is_compressed = (half_instruction.has_value() && (half_instruction->instruction & 0b11) != 0b11);
    if (!is_compressed) instruction = bus.read_32(pc);

    if (instruction.has_value())
    {
        char buf[80] = { 0 };
        disasm_inst(buf, sizeof(buf), rv64, pc, instruction->instruction);
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
        exception != Exception::EnvironmentCallFromMMode)
    {
        std::cout << "warning: exception occured with id " << (int)exception <<
            ", pc = " << std::hex << pc << ", info = " << std::hex << info <<
            std::dec << std::endl;
    }

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

    if (privilege_level <= PrivilegeLevel::Supervisor && medeleg.should_delegate(cause))
    {
        throw std::runtime_error("todo");
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
        mepc.write(original_pc, *this);

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
            return *bus.read_32(pc);

        case Exception::LoadAccessFault:
            return pc;

        case Exception::StoreOrAMOAccessFault:
            return pc;

        case Exception::EnvironmentCallFromUMode:
        case Exception::EnvironmentCallFromSMode:
        case Exception::EnvironmentCallFromMMode:
            return 0;

        default:
            throw std::runtime_error(
                "unable to determine exception casue - "
                "wrong overload used"
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
