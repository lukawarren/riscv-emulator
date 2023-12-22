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
    // Fetch instruction
    const Instruction instruction = { bus.read_32(pc) };
    const u8 opcode = instruction.get_opcode();
    const u8 funct3 = instruction.get_funct3();

    // Reset x0
    registers[0] = 0;

    // Decode - try base cases first because ECALL and ZICSR overlap
    bool did_find_opcode = opcodes_base(*this, instruction);
    if (!did_find_opcode)
    {
        switch(opcode)
        {
            case OPCODES_ZICSR:
                did_find_opcode = opcodes_zicsr(*this, instruction);
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
            instruction.instruction,
            pc
        ));

    pc += sizeof(u32);
}

void CPU::trace()
{
    // if (pc == 0x800001b0)
    //     for (int i = 0; i < 32; ++i)
    //         std::cout << "x" << i << ": " << std::hex << registers[i] << std::endl;
    std::cout << "0x" << std::hex << pc << std::endl;
}