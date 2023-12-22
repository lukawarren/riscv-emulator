#include "cpu.h"
#include "instruction.h"
#include "opcodes_base.h"
#include "opcodes_zicsr.h"
#include <iostream>
#include <format>

CPU::CPU(const u64 ram_size) : bus(ram_size)
{
    // Set x0 to 0, sp to end of memory and pc to start of RAM
    registers[0] = 0;
    registers[2] = Bus::ram_base + ram_size;
    pc = Bus::ram_base;
}

void CPU::cycle()
{
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

    // TODO: check instruction alignment

    const u8 opcode = instruction->get_opcode();
    const u8 funct3 = instruction->get_funct3();

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

            default:
                break;
        }
    }

    if (!did_find_opcode)
        throw std::runtime_error(std::format(
            "unknown opcode 0x{:x} with funct3 0x{:x} - raw = 0x{:x}, pc = 0x{:x}",
            opcode,
            funct3,
            instruction->instruction,
            pc
        ));

    if (!exception_did_occur)
        pc += sizeof(u32);

    exception_did_occur = false;
}

void CPU::trace()
{
    // if (pc >= 0x80000220 && pc <= 0x80000234) {
    //     for (int i = 0; i < 32; ++i)
    //         std::cout << "x" << i << ": " << std::hex << registers[i] << std::endl;
    // }
    std::cout << (int)privilege_level << ": 0x" << std::hex << pc << std::endl;
}

void CPU::raise_exception(const Exception exception, const u64 info)
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
    exception_did_occur = true;

    if (privilege_level <= PrivilegeLevel::Supervisor && medeleg.should_delegate(exception))
    {
        throw std::runtime_error("todo");
    }
    else
    {
        // Handle trap in machine mode
        privilege_level = PrivilegeLevel::Machine;

        // Set PC to mtvec
        pc = mtvec.read(*this);

        // Set mepc to virtual address of instruction that was interrupted
        mepc.write(original_pc, *this);

        // Set mcause to cause
        mcause.write((u64)exception, *this);

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