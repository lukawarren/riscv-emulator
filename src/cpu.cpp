#include "cpu.h"
#include "instruction.h"
#include "opcodes_base.h"
#include "opcodes_zicsr.h"
#include "opcodes_m.h"
#include "opcodes_a.h"
#include "dtb.h"
#include <iostream>
#include <format>

CPU::CPU(const u64 ram_size, const bool emulating_test) :
    bus(ram_size), emulating_test(emulating_test)
{
    // Set x0 to 0, sp to end of memory and pc to start of RAM
    registers[0] = 0;
    registers[2] = Bus::ram_base + ram_size;
    pc = Bus::programs_base;

    // Load DTB into memory
    const u64 dtb_address = Bus::ram_base + ram_size - sizeof(DTB);
    for (size_t i = 0; i < sizeof(DTB); ++i)
        std::ignore = bus.write_8(dtb_address + i, DTB[i]);

    // TODO: set x11 to DTB pointer and x10 to hart id
    registers[10] = 0;
    registers[11] = dtb_address;
}

void CPU::do_cycle()
{
    // Check is 32-bit aligned (remove when compressed support added)
    if ((pc & 0b11) != 0)
    {
        raise_exception(Exception::InstructionAddressMisaligned, pc);
        return;
    }

    // Fetch instruction... if we can!
    const std::optional<Instruction> instruction = { bus.read_32(pc) };
    if (!instruction)
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
    if (instruction->instruction == 0xffffffff ||
        instruction->instruction == 0)
    {
        raise_exception(Exception::IllegalInstruction, instruction->instruction);
        return;
    }

    const u8 opcode = instruction->get_opcode();
    const u8 funct3 = instruction->get_funct3();
    const u8 funct7 = instruction->get_funct7();

    // Reset x0
    registers[0] = 0;

    // Decode - try base cases first because ECALL and ZICSR overlap
    bool did_find_opcode = opcodes_base(*this, *instruction);
    if (!did_find_opcode)
    {
        switch(opcode)
        {
            case OPCODES_ZICSR:
                did_find_opcode = opcodes_zicsr(*this, *instruction);
                break;

            case OPCODES_M:
            case OPCODES_M_32:
                // Distinguished from OPCODES_BASE_R_TYPE[_32] by funct7
                if (funct7 == OPCODES_M_FUNCT_7)
                    did_find_opcode = opcodes_m(*this, *instruction);
                break;

            case OPCODES_A:
                did_find_opcode = opcodes_a(*this, *instruction);
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
            instruction->get_rs2(),
            instruction->instruction,
            pc
        ));

    if (!trap_did_occur)
        pc += sizeof(u32);

    trap_did_occur = false;
    mcycle.increment(*this);
    minstret.increment(*this);
}

void CPU::trace()
{
    std::cout << (int)privilege_level << ": 0x" << std::hex << pc << std::endl;
}

void CPU::raise_exception(const Exception exception)
{
    raise_exception(exception, get_exception_cause(exception));
}

void CPU::raise_exception(const Exception exception, const u64 cause)
{
    if (exception != Exception::EnvironmentCallFromUMode &&
        exception != Exception::EnvironmentCallFromSMode &&
        exception != Exception::EnvironmentCallFromMMode)
    {
        std::cout << "warning: exception occured with id " << (int)exception <<
            ", pc = " << std::hex << pc << ", cause = " << std::hex << cause <<
            std::dec << std::endl;
    }

    handle_trap((u64)exception, cause, false);
}

void CPU::raise_interrupt(const Interrupt interrupt)
{
    handle_trap((u64)interrupt, 0, true);
}

void CPU::handle_trap(const u64 exception_code, const u64 cause, const bool interrupt)
{
    /*
        By default, all traps at any privilege level are handled in machine mode,
        though a machine-mode handler can redirect traps back to the appropriate
        level with the MRET instruction. To increase performance, implementations
        can provide individual read/write bits within medeleg and mideleg to indicate
        that certain exceptions and interrupts should be processed directly by a
        lower privilege level.
     */

    if (interrupt)
        waiting_for_interrupts = false;

    const u64 original_pc = pc;
    const PrivilegeLevel original_privilege_level = privilege_level;
    trap_did_occur = true;

    if (privilege_level <= PrivilegeLevel::Supervisor && medeleg.should_delegate(exception_code))
    {
        throw std::runtime_error("todo");
    }
    else
    {
        // Handle trap in machine mode
        privilege_level = PrivilegeLevel::Machine;

        // Set PC to mtvec
        pc = *mtvec.read(*this);

        // Set mepc to virtual address of instruction that was interrupted
        mepc.write(original_pc, *this);

        // Set mcause to cause - interrupts have MSB set
        mcause.write((u64)exception_code | ((u64)interrupt << 63), *this);

        // Set mtval to (optional) exception-specific information
        mtval.write(cause, *this);

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
