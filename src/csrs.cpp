#include "csrs.h"
#include "cpu.h"

/*
    Some CSRs require access to the CPU.
    That won't compile unless we have the function implementations in a source
    file! Yes, you really can implement struct methods like this - they're
    analogous to classes.
*/

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
        cpu.raise_exception(Exception::IllegalInstruction, cpu.pc);
        return std::nullopt;
    }

    return cpu.mcycle.read(cpu);
}
