#include "opcodes_zicsr.h"
#include <format>
#include <iostream>

bool opcodes_zicsr(CPU& cpu, const Instruction& instruction)
{
    u8 funct3 = instruction.get_funct3();

    switch (funct3)
    {
        case CSRRW:     csrrw (cpu, instruction); break;
        case CSRRS:     csrrs (cpu, instruction); break;
        case CSRRC:     csrrc (cpu, instruction); break;
        case CSRRWI:    csrrwi(cpu, instruction); break;
        case CSRRSI:    csrrsi(cpu, instruction); break;
        case CSRRCI:    csrrci(cpu, instruction); break;

        default:
            return false;
    }

    return true;
}

bool check_satp_trap(CPU& cpu, const u16 csr_address)
{
    // When TVM=1, attempts to read or write the satp CSR while executing in
    // S-mode will raise an illegal instruction exception
    if (csr_address == CSR_SATP &&
        cpu.privilege_level == PrivilegeLevel::Supervisor &&
        cpu.mstatus.fields.tvm == 1)
    {
        cpu.raise_exception(Exception::IllegalInstruction);
        return false;
    }
    return true;
}

std::optional<u64> read_csr(CPU& cpu, const u16 address)
{
    const u16 csr_address = address & 0xfff;

    if ((csr_address >= CSR_PMPCFG0 && csr_address <= CSR_PMPCFG15) ||
        (csr_address >= CSR_PMPADDR0 && csr_address <= CSR_PMPADDR63))
    {
        std::cout << "TODO: pmpaddr*" << std::endl;
        return 0;
    }

    // Debug registers
    if (csr_address >= CSR_DEBUG_BEGIN && csr_address <= CSR_DEBUG_END)
    {
        if ((csr_address <= CSR_DEBUG_LIMIT && cpu.privilege_level >= PrivilegeLevel::Machine) ||
            cpu.privilege_level == PrivilegeLevel::Debug)
            return cpu.debug_registers[csr_address - CSR_DEBUG_BEGIN].read(cpu);
        else
        {
            cpu.raise_exception(Exception::IllegalInstruction);
            return std::nullopt;
        }
    }

    // Check privilege level
    if (CSR::get_privilege_level(address) > cpu.privilege_level)
    {
        cpu.raise_exception(Exception::IllegalInstruction);
        return std::nullopt;
    }

    if (!check_satp_trap(cpu, csr_address))
        return std::nullopt;

    switch(csr_address)
    {
        // TODO: raises exception when mstatus has certain value

        case CSR_SSTATUS:       return cpu.sstatus.read(cpu);
        case CSR_SCOUNTER_EN:   return cpu.scounteren.read(cpu);
        case CSR_SEPC:          return cpu.sepc.read(cpu);
        case CSR_SATP:          return cpu.satp.read(cpu);
        case CSR_MSTATUS:       return cpu.mstatus.read(cpu);
        case CSR_MISA:          return cpu.misa.read(cpu);
        case CSR_MEDELEG:       return cpu.medeleg.read(cpu);
        case CSR_MIDELEG:       return cpu.mideleg.read(cpu);
        case CSR_MIE:           return cpu.mie.read(cpu);
        case CSR_MTVEC:         return cpu.mtvec.read(cpu);
        case CSR_MCOUNTER_EN:   return cpu.mcounteren.read(cpu);
        case CSR_MSCRATCH:      return cpu.mscratch.read(cpu);
        case CSR_MEPC:          return cpu.mepc.read(cpu);
        case CSR_MCAUSE:        return cpu.mcause.read(cpu);
        case CSR_MTVAL:         return cpu.mtval.read(cpu);
        case CSR_MIP:           return cpu.mip.read(cpu);
        case CSR_MTINST:        return cpu.mtinst.read(cpu);
        case CSR_MTVAL2:        return cpu.mtval2.read(cpu);
        case CSR_MNSTATUS:      return 0; // Part of Smrnmi; needed for riscv-tests
        case CSR_MCYCLE:        return cpu.mcycle.read(cpu);
        case CSR_MINSTRET:      return cpu.minstret.read(cpu);
        case CSR_CYCLE:         return cpu.cycle.read(cpu);
        case CSR_INSTRET:       return cpu.instret.read(cpu);
        case CSR_MVENDOR_ID:    return cpu.mvendorid.read(cpu);
        case CSR_MARCH_ID:      return cpu.marchid.read(cpu);
        case CSR_MIMP_ID:       return cpu.mimpid.read(cpu);
        case CSR_MHART_ID:      return cpu.mhartid.read(cpu);

        default:
            throw std::runtime_error("unknown csr read " + std::format("0x{:x}", csr_address));
    }
}

bool write_csr(CPU& cpu, const u64 value, const u16 address)
{
    const u16 csr_address = address & 0xfff;

    // Check we're not read-only and actually have permission
    if (CSR::is_read_only(csr_address) ||
        cpu.privilege_level < CSR::get_privilege_level(csr_address))
    {
        cpu.raise_exception(Exception::IllegalInstruction);
        return false;
    }

    // PMP registers
    if ((csr_address >= CSR_PMPCFG0 && csr_address <= CSR_PMPCFG15) ||
        (csr_address >= CSR_PMPADDR0 && csr_address <= CSR_PMPADDR63))
    {
        std::cout << "TODO: pmp*" << std::endl;
        return true;
    }

    // Debug registers
    if (csr_address >= CSR_DEBUG_BEGIN && csr_address <= CSR_DEBUG_END)
    {
        // To tell programs that breakpoints are not supported,
        // tdata1 must not be writable, and must always read zero.
        if (csr_address == CSR_TDATA1)
            return true;

        if ((csr_address <= CSR_DEBUG_LIMIT && cpu.privilege_level >= PrivilegeLevel::Machine) ||
            cpu.privilege_level == PrivilegeLevel::Debug)
        {
            cpu.debug_registers[csr_address - CSR_DEBUG_BEGIN].write(value, cpu);
            return true;
        }

        // In machine mode but it's a debug-only debug register!
        cpu.raise_exception(Exception::IllegalInstruction);
        return false;
    }

    if (!check_satp_trap(cpu, csr_address))
        return false;

    switch(csr_address)
    {
        // TODO: raises exception when mstatus has certain value (maybe? well is for reads idk)
        case CSR_SSTATUS:       return cpu.sstatus.write(value, cpu);
        case CSR_SCOUNTER_EN:   return cpu.scounteren.write(value, cpu);
        case CSR_SEPC:          return cpu.sepc.write(value, cpu);
        case CSR_SATP:          return cpu.satp.write(value, cpu);
        case CSR_MSTATUS:       return cpu.mstatus.write(value, cpu);
        case CSR_MISA:          return cpu.misa.write(value, cpu);
        case CSR_MEDELEG:       return cpu.medeleg.write(value, cpu);
        case CSR_MIDELEG:       return cpu.mideleg.write(value, cpu);
        case CSR_MIE:           return cpu.mie.write(value, cpu);
        case CSR_MTVEC:         return cpu.mtvec.write(value, cpu);
        case CSR_MCOUNTER_EN:   return cpu.mcounteren.write(value, cpu);
        case CSR_MSCRATCH:      return cpu.mscratch.write(value, cpu);
        case CSR_MEPC:          return cpu.mepc.write(value, cpu);
        case CSR_MCAUSE:        return cpu.mcause.write(value, cpu);
        case CSR_MTVAL:         return cpu.mtval.write(value, cpu);
        case CSR_MIP:           return cpu.mip.write(value, cpu);
        case CSR_MTINST:        return cpu.mtinst.write(value, cpu);
        case CSR_MTVAL2:        return cpu.mtval2.write(value, cpu);
        case CSR_MNSTATUS:      return true; // Part of Smrnmi; needed for riscv-tests
        case CSR_MCYCLE:        return cpu.mcycle.write(value, cpu);
        case CSR_CYCLE:         return cpu.cycle.write(value, cpu);
        case CSR_MVENDOR_ID:    return cpu.mvendorid.write(value, cpu);
        case CSR_MARCH_ID:      return cpu.marchid.write(value, cpu);
        case CSR_MIMP_ID:       return cpu.mimpid.write(value, cpu);
        case CSR_MHART_ID:      return cpu.mhartid.write(value, cpu);

        default:
            throw std::runtime_error("unknown csr write " + std::format("0x{:x}", csr_address));
    }
}

void csrrw(CPU& cpu, const Instruction& instruction)
{
    /*
        If rd=x0, then the instruction shall not read the CSR and shall not
        cause any of the side-effects that might occur on a CSR read
     */

    const u64 address = instruction.get_imm(Instruction::Type::I);
    const std::optional<u64> value = (instruction.get_rd() != 0) ? read_csr(cpu, address) : 0;
    if (!value) return;

    // Write rs1 to CSR
    if (!write_csr(cpu, cpu.registers[instruction.get_rs1()], address))
        return;

    // Put old value of CSR into rd
    if (instruction.get_rd() != 0)
        cpu.registers[instruction.get_rd()] = *value;
}

void csrrs(CPU& cpu, const Instruction& instruction)
{
    // Write CSR to rd
    const u64 address = instruction.get_imm(Instruction::Type::I);
    const std::optional<u64> csr = read_csr(cpu, address);
    if (!csr) return;

    // If rs1=x0, then the instruction will not write to the CSR at all
    if (instruction.get_rs1() != 0)
    {
        // Initial value in rs1 is treated as a bit mask that specifies bit
        // positions to be set in the CSR. Any bit that is high in rs1 will cause
        // the corresponding bit to be set in the CSR, if that CSR is writable.
        // Other bits are unaffected (though may have side effects when written).
        const u64 bitmask = cpu.registers[instruction.get_rs1()];
        if (!write_csr(cpu, (*csr) | bitmask, address))
            return;
    }

    cpu.registers[instruction.get_rd()] = *csr;
}

void csrrc(CPU& cpu, const Instruction& instruction)
{
    // Write CSR to rd
    const u64 address = instruction.get_imm(Instruction::Type::I);
    const std::optional<u64> csr = read_csr(cpu, address);
    if (!csr) return;

    // If rs1=x0, then the instruction will not write to the CSR at all
    if (instruction.get_rs1() != 0)
    {
        // Like csrrs but instead of setting, we're clearing
        const u64 bitmask = cpu.registers[instruction.get_rs1()];
        if (!write_csr(cpu, (*csr) & (~bitmask), address))
            return;
    }

    cpu.registers[instruction.get_rd()] = *csr;
}

void csrrwi(CPU& cpu, const Instruction& instruction)
{
    /*
        If rd=x0, then the instruction shall not read the CSR and shall not
        cause any of the side-effects that might occur on a CSR read
     */

    const u64 address = instruction.get_imm(Instruction::Type::I);
    const std::optional<u64> csr = (instruction.get_rd() != 0) ? read_csr(cpu, address) : 0;
    if (!csr) return;

    // Write value in rs1 directly to CSR
    if (!write_csr(cpu, instruction.get_rs1(), address))
        return;

    // Put old value of CSR into rd
    if (instruction.get_rd() != 0)
        cpu.registers[instruction.get_rd()] = *csr;
}

void csrrsi(CPU& cpu, const Instruction& instruction)
{
    // Read CSR
    const u64 address = instruction.get_imm(Instruction::Type::I);
    const std::optional<u64> csr = read_csr(cpu, address);
    if (!csr) return;

    // OR with raw value of rs1, but only if rs1!=0
    if (instruction.get_rs1() != 0)
        if (!write_csr(cpu, *csr | instruction.get_rs1(), address))
            return;

    cpu.registers[instruction.get_rd()] = *csr;
}

void csrrci(CPU& cpu, const Instruction& instruction)
{
    // Read CSR
    const u64 address = instruction.get_imm(Instruction::Type::I);
    const std::optional<u64> csr = read_csr(cpu, address);
    if (!csr) return;

    // Again, only if rs1!=0
    if (instruction.get_rs1() != 0)
        if (!write_csr(cpu, *csr & (~instruction.get_rs1()), address))
            return;

    cpu.registers[instruction.get_rd()] = *csr;
}
