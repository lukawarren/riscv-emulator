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
        return ((instruction >> 7) & 0b11111);
    }

    u8 get_rd_with_offset() const
    {
        return ((instruction >> 7) & 0b111) + register_offset;
    }

    u8 get_rs1() const
    {
        // Same place as rd
        return get_rd();
    }

    u8 get_rs2() const
    {
        // 5 bits; 2-6 inclusive
        return ((instruction >> 2) & 0b11111);
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
                return ((instruction << 1) & 0x40) // imm[6]
                     | ((instruction >> 7) & 0x38) // imm[5:3]
                     | ((instruction >> 4) & 0x4);

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

    u64 get_none_zero_imm() const
    {
        u64 nzimm = ((instruction >> 7) & 0x20) | ((instruction >> 2) & 0x1f);

        // Sign extend
        if ((nzimm & 0x20) == 0) return nzimm;
        else return (u64)(i64)(i8)(0xc0 | nzimm);
    }

    u64 get_lui_non_zero_imm() const
    {
        u64 nzimm = ((instruction << 5) & 0x20000) | ((instruction << 10) & 0x1f000);

        // Sign-extend
        if ((nzimm & 0x20000) == 0) return nzimm;
        else return (u64)(i64)(i32)(0xfffc0000 | nzimm);
    }

    u64 get_addi16sp_none_zero_imm() const
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

    u64 get_addi4spn_none_zero_unsigned_imm() const
    {
        u64 nzuimm = ((instruction >> 1) & 0x3c0) // znuimm[9:6]
                    | ((instruction >> 7) & 0x30) // znuimm[5:4]
                    | ((instruction >> 2) & 0x8)  // znuimm[3]
                    | ((instruction >> 4) & 0x4); // znuimm[2]

        // TODO: raise illegal instruction exception
        assert(nzuimm != 0);
        return nzuimm;
    }

    u64 get_ld_sd_imm() const
    {
        return ((instruction << 1) & 0xc0) | // imm[7:6]
                ((instruction >> 7) & 0x38);
    }

    u64 get_jump_offset() const
    {
        u16 offset =
              ((instruction >> 1) & 0x800) // offset[11]
            | ((instruction << 2) & 0x400) // offset[10]
            | ((instruction >> 1) & 0x300) // offset[9:8]
            | ((instruction << 1) & 0x80)  // offset[7]
            | ((instruction >> 1) & 0x40)  // offset[6]
            | ((instruction << 3) & 0x20)  // offset[5]
            | ((instruction >> 7) & 0x10)  // offset[4]
            | ((instruction >> 2) & 0x0e); // offset[3:1]

        // Sign extend
        if ((offset & 0x800) == 0) return offset;
        else return (u64)(i64)(i16)(0xf000 | offset);
    }

    u64 get_branch_offset() const
    {
        u16 offset =
              ((instruction >> 4) & 0x100) // offset[8]
            | ((instruction << 1) & 0xc0)  // offset[7:6]
            | ((instruction << 3) & 0x20)  // offset[5]
            | ((instruction >> 7) & 0x18)  // offset[4:3]
            | ((instruction >> 2) & 0x6);  // offset[2:1]

        // Sign extend
        if ((offset & 0x100) == 0) return offset;
        else return (u64)(i64)(i16)(0xfe00 | offset);
    }

    u64 get_lwsp_offset() const
    {
        return ((instruction << 4) & 0xc0)  // offset[7:6]
             | ((instruction >> 7) & 0x20)  // offset[5]
             | ((instruction >> 2) & 0x1c); // offset[4:2]
    }

    u64 get_ldsp_offset() const
    {
        return ((instruction << 4) & 0x1c0) // offset[8:6]
             | ((instruction >> 7) & 0x20)  // offset[5]
             | ((instruction >> 2) & 0x18); // offset[4:3]
    }

    u64 get_swsp_offset() const
    {
        return ((instruction >> 1) & 0xc0)  // offset[7:6]
             | ((instruction >> 7) & 0x3c); // offset[5:2]
    }

    u64 get_sdsp_offset() const
    {
        return ((instruction >> 1) & 0x1c0) // offset[8:6]
             | ((instruction >> 7) & 0x38); // offset[5:3]
    }

    u16 get_jump_target() const
    {
        // Bits 2-12
        return (instruction >> 2) & 0b11111111111;
    }

    u8 get_funct2() const
    {
        return (instruction >> 10) & 0x3;
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

    u64 get_shamt() const
    {
        return ((instruction >> 7) & 0x20) | ((instruction >> 2) & 0x1f);
    }
};
