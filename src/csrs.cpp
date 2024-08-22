#include "csrs.h"
#include "cpu.h"
#include "instruction.h"
#include "opcodes_f.h"

bool FCSR::write(const u64 value, CPU& cpu)
{
    if (!check_fs_field(cpu, true))
        return true;

    /*
        Bits 31–8 of the fcsr are reserved for other standard extensions,
        including the "L" standard extension for decimal floating-point.
        If these extensions are not present, implementations shall
        ignore writes to these bits and supply a zero value when read.
    */
    bits = value & 0b00000000000000000000000011111111;
    return true;
}

std::optional<u64> FCSR::read(CPU& cpu)
{
    if (!check_fs_field(cpu, false))
        return true;

    return bits;
}

/*
    For the below there is no need to do the full check (like above)
    because we already know RV64F is already enabled (as these are only
    called by opcodes), so there can't possibly be an exception.
*/
void FCSR::set_nx(CPU& cpu) { bits |=  (1 << 0); cpu.mstatus.fields.fs = 3; }
void FCSR::set_uf(CPU& cpu) { bits |=  (1 << 1); cpu.mstatus.fields.fs = 3; }
void FCSR::set_of(CPU& cpu) { bits |=  (1 << 2); cpu.mstatus.fields.fs = 3; }
void FCSR::set_dz(CPU& cpu) { bits |=  (1 << 3); cpu.mstatus.fields.fs = 3; }
void FCSR::set_nv(CPU& cpu) { bits |=  (1 << 4); cpu.mstatus.fields.fs = 3; }

bool FSFlags::write(const u64 value, CPU& cpu)
{
    if (!check_fs_field(cpu, true))
        return true;

    cpu.fcsr.set_fflags(value);
    return true;
}

std::optional<u64> FSFlags::read(CPU& cpu)
{
    if (!check_fs_field(cpu, false))
        return true;

    return cpu.fcsr.get_fflags();
}

bool FRM::write(const u64 value, CPU& cpu)
{
    if (!check_fs_field(cpu, true))
        return true;

    cpu.fcsr.set_rounding_mode(value);
    return true;
}

std::optional<u64> FRM::read(CPU& cpu)
{
    if (!check_fs_field(cpu, false))
        return true;

    return cpu.fcsr.get_rounding_mode();
}

bool SIE::write(const u64 value, CPU& cpu)
{
    MIE old_value = cpu.mie;
    cpu.mie.bits = (u16)value;

    // Everything is writable except for MEIE, MTIE and MSIE
    if (old_value.mei()) cpu.mie.set_mei(); else cpu.mie.clear_mei();
    if (old_value.mti()) cpu.mie.set_mti(); else cpu.mie.clear_mti();
    if (old_value.msi()) cpu.mie.set_msi(); else cpu.mie.clear_msi();

    return true;
}

std::optional<u64> SIE::read(CPU& cpu)
{
    MIE new_value = cpu.mie;

    //  MEIE, MTIE and MSIE are WPRI and reserved for future use
    new_value.clear_mei();
    new_value.clear_mti();
    new_value.clear_msi();

    return new_value.bits;
}

bool SIP::write(const u64 value, CPU& cpu)
{
    MIP new_value;
    new_value.bits = value;

    // All bits besides SSIP, USIP, and UEIP in the sip register are read-only
    if (new_value.ssi()) cpu.mip.set_ssi(); else cpu.mip.clear_ssi();
    if (new_value.usi()) cpu.mip.set_usi(); else cpu.mip.clear_usi();
    if (new_value.uei()) cpu.mip.set_uei(); else cpu.mip.clear_uei();

    return true;
}

std::optional<u64> SIP::read(CPU& cpu)
{
    //  MEIE, MTIE and MSIE are WIRI; can do same as with SIE
    MIP new_value = cpu.mip;
    new_value.clear_mei();
    new_value.clear_mti();
    new_value.clear_msi();
    return new_value.bits;
}

bool SStatus::write(const u64 value, CPU& cpu)
{
    // Don't set the wpri fields; keep them zero (XS is read-only)
    // SXL and UXL are already hard-wired
    MStatus::Fields& fields = cpu.mstatus.fields;
    fields.mxr = (value >> 19) & 0x1;
    fields.sum = (value >> 18) & 0x1;
    fields.fs = (value >> 13) & 0x3;
    fields.vs = (value >> 9) & 0x3;
    fields.spp = (value >> 8) & 0x1;
    fields.ube = (value >> 6) & 0x1;
    fields.spie = (value >> 5) & 0x1;
    fields.sie = (value >> 1) & 0x1;
    fields.sd = (fields.fs == 0b11) || (fields.xs == 0b11);

    return true;
}

std::optional<u64> SStatus::read(CPU& cpu)
{
    MStatus::Fields& fields = cpu.mstatus.fields;
    fields.sd = (fields.fs == 0b11) || (fields.xs == 0b11);

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
    return (mxl << 62) | extensions;
}

bool SATP::write(const u64 value, CPU& cpu)
{
    u64 old_bits = bits;
    bits = value;

    // "if satp is written with an unsupported MODE, the entire write has no
    // effect; no fields in satp are modified"
    if (get_mode() != ModeSettings::None && get_mode() != ModeSettings::Sv39)
        bits = old_bits;

    cpu.invalidate_tlb();

    return true;
}

std::optional<u64> SATP::read(CPU& cpu)
{
    return bits;
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
    // Work out CSR address for nice error message
    const Instruction instruction = *cpu.read_32(cpu.pc);
    const u64 address = instruction.get_imm(Instruction::Type::I) & 0xfff;
    throw std::runtime_error(std::format(
        "unimplemented CSR with address 0x{:x}, value = 0x{:x}",
        address,
        value
    ));
}
