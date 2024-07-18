#include "csrs.h"
#include "cpu.h"
#include "instruction.h"

/*
    Some CSRs require access to the CPU.
    That won't compile unless we have the function implementations in a source file!
*/

bool SStatus::write(const u64 value, CPU& cpu)
{
    // Don't set the wpri fields; keep them zero
    MStatus::Fields& fields = cpu.mstatus.fields;
    fields.sd = (value >> 63) & 0x1;
    fields.uxl = (value >> 32) & 0x3;
    fields.mxr = (value >> 19) & 0x1;
    fields.sum = (value >> 18) & 0x1;
    fields.xs = (value >> 15) & 0x3;
    fields.fs = (value >> 13) & 0x3;
    fields.vs = (value >> 9) & 0x3;
    fields.spp = (value >> 8) & 0x1;
    fields.ube = (value >> 6) & 0x1;
    fields.spie = (value >> 5) & 0x1;
    fields.sie = (value >> 1) & 0x1;
    return true;
}

std::optional<u64> SStatus::read(CPU& cpu)
{
    const MStatus::Fields& fields = cpu.mstatus.fields;
    u64 value = 0;
    value |= ((u64)(fields.sd) << 63);
    value |= ((u64)(fields.wpri_1) << 34);
    value |= ((u64)(fields.uxl) << 32);
    value |= ((u64)(fields.wpri_2) << 20);
    value |= ((u64)(fields.mxr) << 19);
    value |= ((u64)(fields.sum) << 18);
    value |= ((u64)(fields.wpri_3) << 17);
    value |= ((u64)(fields.xs) << 15);
    value |= ((u64)(fields.fs) << 13);
    value |= ((u64)(fields.wpri_4) << 11);
    value |= ((u64)(fields.vs) << 9);
    value |= ((u64)(fields.spp) << 8);
    value |= ((u64)(fields.wpri_5) << 7);
    value |= ((u64)(fields.ube) << 6);
    value |= ((u64)(fields.spie) << 5);
    value |= ((u64)(0) << 2); // wpri_6
    value |= ((u64)(fields.sie) << 1);
    value |= ((u64)(0) << 0); // wpri_7
    return value;
}

bool MISA::write(const u64, CPU&)
{
    // WARL - see read()
    return true;
}

std::optional<u64> MISA::read(CPU& cpu)
{
    // All fields are WARL - program can write whatever it wants but must always
    // read correct values (i.e. can write bogus values, then read to get CPU
    // capabilities)
    const u64 mxl = 2; // XLEN = 64
    const u64 extensions = CPU::get_supported_extensions();
    return (mxl << 62)| extensions;
}

bool Cycle::write(const u64, CPU&)
{
    return true;
}

std::optional<u64> Cycle::read(CPU& cpu)
{
    // Read-only shadow of mcycle
    if (cpu.privilege_level < PrivilegeLevel::Machine &&
        !cpu.mcounteren.is_cycle_enabled())
    {
        cpu.raise_exception(Exception::IllegalInstruction);
        return std::nullopt;
    }

    return cpu.mcycle.read(cpu);
}

std::optional<u64> InstRet::read(CPU& cpu)
{
    // Read-only shadow of mcycle
    if (cpu.privilege_level < PrivilegeLevel::Machine &&
        !cpu.mcounteren.is_instret_enabled())
    {
        cpu.raise_exception(Exception::IllegalInstruction);
        return std::nullopt;
    }

    return cpu.minstret.read(cpu);
}

bool UnimplementedCSR::write(const u64 value, CPU& cpu)
{
    if (value == 0) return true;

    // Work out CSR address for nice error message
    const Instruction instruction = *cpu.read_32(cpu.pc);
    const u64 address = instruction.get_imm(Instruction::Type::I) & 0xfff;
    throw std::runtime_error(std::format(
        "unimplemented CSR with address 0x{:x}, value = 0x{:x}",
        address,
        value
    ));
}
