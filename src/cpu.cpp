#include "cpu.h"
#include "instruction.h"
#include "opcodes_base.h"
#include "opcodes_zicsr.h"
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
        throw std::runtime_error(
                "unknown opcpode " +
                std::to_string(opcode) +
                " with funct3 " +
                std::to_string(funct3)
            );

    pc += sizeof(u32);
}

void CPU::trace()
{
    std::cout << "0x" << std::hex << pc << std::endl;
}