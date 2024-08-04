#pragma once
#include "common.h"

/*
    Instruction types:
    - R-Type: Register type instructions
    - I-Type: Immediate type instructions
    - S-Type: Store type instructions
    - B-Type: Break type instructions
    - U-Type: Register type instructions
    - J-Type: Jump type instructions

    Encoded values:
    - opcode:   Specifies the instruction code
    - rd:       Destination register
    - funct3:   Distinguishes between different instructions with same opcode
    - funct7:   As above, but 7 bits as opposed to 3
    - rs1:      Address of source register 1
    - rs2:      Address of source register 2
    - imm:      Address of destination register
    - shamt:    Shift amount for shift instructions
*/
struct Instruction
{
    enum class Type
    {
        R, I, S, B, U, J
    };

    uint32_t instruction;

    Instruction(const uint32_t instruction)
    {
        this->instruction = instruction;
    }

    u8 get_opcode() const
    {
        return instruction & 0b1111111;
    }

    u8 get_rd() const
    {
        return (instruction >> 7) & 0b11111;
    }

    u8 get_funct2() const
    {
        return (instruction & 0x03000000) >> 25;
    }

    u8 get_funct3() const
    {
        return (instruction >> 12) & 0b111;
    }

    u8 get_funct7() const
    {
        return (instruction >> 25) & 0b1111111;
    }

    u8 get_rs1() const
    {
        return (instruction >> 15) & 0b11111;
    }

    u8 get_rs2() const
    {
        return (instruction >> 20) & 0b11111;
    }

    u8 get_rs3() const
    {
        return (instruction >> 27) & 0b11111;
    }

    u8 get_rounding_mode() const
    {
        return (instruction >> 12) & 0b111;
    }

    u8 get_rs2_6_bits() const
    {
        return (instruction >> 20) & 0b111111;
    }

    u64 get_imm(const Type type) const
    {
        switch (type)
        {
            case Type::I:
                // Sign extend then shift down
                return ((i64)(i32)instruction) >> 20;

            case Type::S:
                // Sign extend then shift down, then OR with lower bits
                return ((i64)(i32)(instruction & 0xfe000000) >> 20) |
                    ((instruction >> 7) & 0x1f);

            case Type::B:
                return ((i64)(i32)(instruction & 0x80000000) >> 19)
                | ((instruction & 0x80) << 4)
                | ((instruction >> 20) & 0x7e0)
                | ((instruction >> 7) & 0x1e);

            case Type:: U:
                // No need to shift right by 12 as all opcodes using it
                // ultimately then shift left by 12
                return ((i64)(i32)(instruction & 0xfffff000));

            case Type::J:
                return (u64)((i64)(i32)(instruction & 0x80000000) >> 11)
                | (instruction & 0xff000)
                | ((instruction >> 9) & 0x800)
                | ((instruction >> 20) & 0x7fe);

            case Type::R:
            default:
                throw std::runtime_error("unspported instruction type");
                return 0;
        }
    }

    u8 get_shamt() const
    {
        // shamt is 6 bits for RV64I, but 5 bits for RV32I
        return (instruction >> 20) & 0b111111;
    }
};