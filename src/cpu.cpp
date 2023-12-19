#include "cpu.h"
#include "instruction.h"
#include "opcodes_base.h"
#include <iostream>

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

    // Reset x0
    registers[0] = 0;
    opcode_base(*this, instruction);
    pc += sizeof(u32);
}

void CPU::trace()
{
    for (int i = 0; i < 32; ++i)
        std::cout << "x" << i << ": " << registers[i] << std::endl;
    std::cout << "pc: " << pc << std::endl;
}