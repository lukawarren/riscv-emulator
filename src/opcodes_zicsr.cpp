#include "opcodes_zicsr.h"
#include <format>
#include <iostream>

bool opcodes_zicsr(CPU& cpu, const Instruction& instruction)
{
    u8 funct3 = instruction.get_funct3();

    switch (funct3)
    {
        case CSRRS:     csrrs (cpu, instruction); break;
        case CSRRW:     csrrw (cpu, instruction); break;
        case CSRRWI:    csrrwi(cpu, instruction); break;

        default:
            return false;
    }

    return true;
}

u64 read_csr(CPU& cpu, const u16 address)
{
    if ((address >= CSR_PMPCFG0 && address <= CSR_PMPCFG15) ||
        (address >= CSR_PMPADDR0 && address <= CSR_PMPADDR63)) {
        std::cout << "TODO: pmpaddr*" << std::endl;
        return 0;
    }

    // TODO: check privilege level, throw exceptions, etc.
    switch(address & 0xfff)
    {
        // TODO: raises exception when mstatus has certain value
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
        case CSR_MHARTID:       return cpu.mhartid.read(cpu);

        default:
            throw std::runtime_error("unknown csr read " + std::format("0x{:x}", address & 0xfff));
    }
}

void write_csr(CPU& cpu, const u64 value, const u16 address)
{
    if ((address >= CSR_PMPCFG0 && address <= CSR_PMPCFG15) ||
        (address >= CSR_PMPADDR0 && address <= CSR_PMPADDR63)) {
        std::cout << "TODO: pmp*" << std::endl;
        return;
    }

    if (CSR::is_read_only(address)) {
        // TODO: illegal instruction exception
        return;
    }

    // TODO: check privilege level, throw exceptions, etc.

    std::cout << "value = " << value << std::endl;

    switch(address & 0xfff)
    {
        // TODO: raises exception when mstatus has certain value (maybe? well is for reads idk)
        case CSR_SATP:          cpu.satp.write(value, cpu);         break;
        case CSR_MSTATUS:       cpu.mstatus.write(value, cpu);      break;
        case CSR_MISA:          cpu.misa.write(value, cpu);         break;
        case CSR_MEDELEG:       cpu.medeleg.write(value, cpu);      break;
        case CSR_MIDELEG:       cpu.mideleg.write(value, cpu);      break;
        case CSR_MIE:           cpu.mie.write(value, cpu);          break;
        case CSR_MTVEC:         cpu.mtvec.write(value, cpu);        break;
        case CSR_MCOUNTER_EN:   cpu.mcounteren.write(value, cpu);   break;
        case CSR_MSCRATCH:      cpu.mscratch.write(value, cpu);     break;
        case CSR_MEPC:          cpu.mepc.write(value, cpu);         break;
        case CSR_MCAUSE:        cpu.mcause.write(value, cpu);       break;
        case CSR_MTVAL:         cpu.mtval.write(value, cpu);        break;
        case CSR_MIP:           cpu.mip.write(value, cpu);          break;
        case CSR_MTINST:        cpu.mtinst.write(value, cpu);       break;
        case CSR_MTVAL2:        cpu.mtval2.write(value, cpu);       break;
        case CSR_MNSTATUS:                                          break; // Part of Smrnmi; needed for riscv-tests
        case CSR_MHARTID:       cpu.mhartid.write(value, cpu);      break;

        default:
            throw std::runtime_error("unknown csr write " + std::format("0x{:x}", address & 0xfff));
    }
}

void csrrw(CPU& cpu, const Instruction& instruction)
{
    /*
        If rd=x0, then the instruction shall not read the CSR and shall not
        cause any of the side-effects that might occur on a CSR read
     */

    const u64 address = instruction.get_imm(Instruction::Type::I);
    const u64 value = (instruction.get_rd() != 0) ? read_csr(cpu, address) : 0;

    // Write rs1 to CSR
    write_csr(cpu, { cpu.registers[instruction.get_rs1()] }, address);

    // Put old value of CSR into rd
    if (instruction.get_rd() != 0)
        cpu.registers[instruction.get_rd()] = value;
}

void csrrs(CPU& cpu, const Instruction& instruction)
{
    // Write CSR to rd
    const u64 address = instruction.get_imm(Instruction::Type::I);
    const u64 csr = read_csr(cpu, address);
    cpu.registers[instruction.get_rd()] = csr;

    // Initial value in rs1 is treated as a bit mask that specifies bit
    // positions to be set in the CSR. Any bit that is high in rs1 will cause
    // the corresponding bit to be set in the CSR, if that CSR is writable.
    // Other bits are unaffected (though may have side effects when written).
    if (!CSR::is_read_only(address))
    {
        const u64 bitmask = cpu.registers[instruction.get_rs1()];
        write_csr(cpu, csr | bitmask, address);
    }
}

void csrrwi(CPU& cpu, const Instruction& instruction)
{
    /*
        If rd=x0, then the instruction shall not read the CSR and shall not
        cause any of the side-effects that might occur on a CSR read
     */

    const u64 address = instruction.get_imm(Instruction::Type::I);
    const u64 csr = (instruction.get_rd() != 0) ? read_csr(cpu, address) : 0;

    // Write value in rs1 directly to CSR
    write_csr(cpu, { instruction.get_rs1() }, address);

    // Put old value of CSR into rd
    if (instruction.get_rd() != 0)
        cpu.registers[instruction.get_rd()] = csr;
}
