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

CSR read_csr(CPU& cpu, const u16 address)
{
    if ((address >= CSR_PMPCFG0 && address <= CSR_PMPCFG15) ||
        (address >= CSR_PMPADDR0 && address <= CSR_PMPADDR63)) {
        std::cout << "TODO: pmpaddr*" << std::endl;
        return { 0 } ;
    }

    // TODO: check privilege level, throw exceptions, etc.
    switch(address & 0xfff)
    {
        case CSR_SATP: return 0; // TODO: raises exception when mstatus has certain value

        case CSR_MSTATUS: return 0;
        case CSR_MEDELEG: return 0;
        case CSR_MIDELEG: return 0;
        case CSR_MIE: return 0;

        case CSR_MTVEC: return cpu.mtvec;
        case CSR_MEPEC: return cpu.mepec;

        // Hart ID - only one "core" for now
        case CSR_MHARTID: return cpu.hartid;

        default:
            throw std::runtime_error("unknown csr read " + std::format("0x{:x}", address & 0xfff));
    }
}

void write_csr(CPU& cpu, const CSR& csr, const u16 address)
{
    if ((address >= CSR_PMPCFG0 && address <= CSR_PMPCFG15) ||
        (address >= CSR_PMPADDR0 && address <= CSR_PMPADDR63)) {
        std::cout << "TODO: pmp*" << std::endl;
        return;
    }

    // TODO: check privilege level, RW access, throw exceptions, etc.
    std::cout << "Wrote 0x" << std::hex << csr.data << " to CSR 0x" << std::hex << address << std::endl;
    switch(address & 0xfff)
    {
        case CSR_SATP:
            // Virtual memory for supervisor - 0 means it's disabled
            if (csr.data != 0)
                throw std::runtime_error("paging not supported; tried to enable");
        break;

        case CSR_MSTATUS:
            if (csr.data != 0)
                throw std::runtime_error("mstatus not supported");
        break;

        case CSR_MEDELEG:
        case CSR_MIDELEG:
        case CSR_MIE:
            // Each bit configures an interrupt
            if (csr.data != 0)
                throw std::runtime_error("interrupts not supported; tried to enable");
        break;

        case CSR_MTVEC: cpu.mtvec = csr; break;
        case CSR_MEPEC: cpu.mepec = csr; break;

        case CSR_MNSTATUS: cpu.mnstatus = csr; break;

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

    u64 address = instruction.get_imm(Instruction::Type::I);
    CSR csr = (instruction.get_rd() != 0) ? read_csr(cpu, address) : CSR { 0 };

    // Write rs1 to CSR
    write_csr(cpu, { cpu.registers[instruction.get_rs1()] }, address);

    // Put old value of CSR into rd
    if (instruction.get_rd() != 0)
        cpu.registers[instruction.get_rd()] = csr.data;
}

void csrrs(CPU& cpu, const Instruction& instruction)
{
    // Write CSR to rd
    u64 address = instruction.get_imm(Instruction::Type::I);
    CSR csr = read_csr(cpu, address);
    cpu.registers[instruction.get_rd()] = csr.data;

    // Initial value in rs1 is treated as a bit mask that specifies bit
    // positions to be set in the CSR. Any bit that is high in rs1 will cause
    // the corresponding bit to be set in the CSR, if that CSR is writable.
    // Other bits are unaffected (though may have side effects when written).
    if (!csr.is_read_only(address))
    {
        const u64 bitmask = cpu.registers[instruction.get_rs1()];
        csr.data |= bitmask;
        write_csr(cpu, csr, address);
    }
}

void csrrwi(CPU& cpu, const Instruction& instruction)
{
    /*
        If rd=x0, then the instruction shall not read the CSR and shall not
        cause any of the side-effects that might occur on a CSR read
     */

    u64 address = instruction.get_imm(Instruction::Type::I);
    CSR csr = (instruction.get_rd() != 0) ? read_csr(cpu, address) : CSR { 0 };

    // Write value in rs1 directly to CSR
    write_csr(cpu, { instruction.get_rs1() }, address);

    // Put old value of CSR into rd
    if (instruction.get_rd() != 0)
        cpu.registers[instruction.get_rd()] = csr.data;
}
