#pragma once
#include "types.h"
#include <stdexcept>
#include <cassert>

/*
    Compressed instructions differ in that they are 16-bits
    as oppose to 32.

    Some instructions have rd, rs1, rs2, etc. whereas others
    are encoded with rd', rs1' and rs2'. These are 3 bits wide
    as opposed to the "usual" 5.
*/
struct CompressedInstruction
{
    // "register 0" actually denotes x8, etc.
    constexpr static uint8_t register_offset = 8;

    enum class Type
    {
        CR,  // Register
        CI,  // Immediate
        CSS, // Stack-relative store
        CIW, // Wide immediate
        CL,  // Load
        CS,  // Store
        CB,  // Branch
        CJ   // Jump
    };

    uint16_t instruction;

    CompressedInstruction(const uint16_t instruction)
    {
        this->instruction = instruction;
    }

    u8 get_opcode() const
    {
        return instruction & 0b11;
    }

    u8 get_rd() const
    {
        // 5 bits; 7-11 inclusive
        return ((instruction >> 7) & 0b11111) + register_offset;
    }

    u8 get_rs1() const
    {
        // Same place as rd
        return get_rd();
    }

    u8 get_rs2() const
    {
        // 5 bits; 2-6 inclusive
        return ((instruction >> 2) & 0b11111) + register_offset;
    }

    u8 get_rd_alt() const
    {
        // 3 bits; 2-4 inclusive
        return ((instruction >> 2) & 0b111) + register_offset;
    }

    u8 get_rs1_alt() const
    {
        // 3 bits; 7-9 inclusive
        return ((instruction >> 7) & 0b111) + register_offset;
    }

    u8 get_rs2_alt() const
    {
        // Same place as rd'
        return get_rd_alt();
    }

    u16 get_imm(const Type type) const
    {
        switch (type)
        {
            case Type::CI:
            {
                // Bit 12 followed by bits 6-2
                const u16 upper = (instruction >> 12) & 0b1;
                const u16 lower = (instruction >> 2) & 0b11111;
                return (upper << 6) | lower;
            }

            case Type::CSS:
                // Bits 7-12
                return (instruction >> 7) & 0b111111;

            case Type::CIW:
                // Bits 5-12
                return (instruction >> 5) & 0b11111111;

            case Type::CL:
            case Type::CS:
            {
                // Bits 10-12 followed by bits 5-6
                const u16 upper = (instruction >> 10) & 0b111;
                const u16 lower = (instruction >> 5) & 0b11;
                return (upper << 2) | lower;
            }

            default:
                throw std::runtime_error("unspported instruction type");
                return 0;
        }
    }

    u64 get_none_zero_imm(const Type type) const
    {
        switch (type)
        {
            case Type::CI:
            {
                u16 nzimm =  ((instruction >> 3) & 0x200)  // nzimm[9]
                            | ((instruction >> 2) & 0x10)  // nzimm[4]
                            | ((instruction << 1) & 0x40)  // nzimm[6]
                            | ((instruction << 4) & 0x180) // nzimm[8:7]
                            | ((instruction << 3) & 0x20); // nzimm[5]

                // Sign-extend
                if ((nzimm & 0x200) != 0)
                    return (u64)(i64)(i32)(i16)(0xfc00 | nzimm);
                else
                    return nzimm;
            }

            default:
                throw std::runtime_error("unspported instruction type");
                return 0;
        }
    }

    u64 get_none_zero_unsigned_imm(const Type type) const
    {
        switch (type)
        {
            case Type::CIW:
            {
                u16 nzuimm =  ((instruction >> 1) & 0x3c0) // nzuimm[9:6]
                            | ((instruction >> 7) & 0x30)  // nzuimm[5:4]
                            | ((instruction >> 2) & 0x8)   // nzuimm[3]
                            | ((instruction >> 4) & 0x4);  // nzuimm[2]

                // TODO: raise error if zero
                assert(nzuimm != 0);
                return nzuimm;
            }

            default:
                throw std::runtime_error("unspported instruction type");
                return 0;
        }
    }

    u8 get_offset() const
    {
        // Bits 10-12 followed by bits 2-6
        const u8 upper = (instruction >> 10) & 0b111;
        const u8 lower = (instruction >> 2) & 0b11111;
        return (upper << 5) | lower;
    }

    u16 get_jump_target() const
    {
        // Bits 2-12
        return (instruction >> 2) & 0b11111111111;
    }

    u8 get_funct3() const
    {
        // Bits 13-15
        return (instruction >> 13) & 0b111;
    }

    u8 get_funct4() const
    {
        // Bits 12-15
        return (instruction >> 12) & 0b1111;
    }

    u8 get_shamt() const
    {
        return get_imm(Type::CI) & 0b11111;
    }
};
