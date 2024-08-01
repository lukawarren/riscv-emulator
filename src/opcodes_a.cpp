#include "opcodes_a.h"

#define GET_ADDRESS() \
const u64 address = cpu.registers[instruction.get_rs1()];

#define CHECK_LOAD_ALIGNMENT_32(addr) if (addr % 4 != 0)\
{\
    cpu.raise_exception(Exception::LoadAddressMisaligned);\
    return;\
}

#define CHECK_LOAD_ALIGNMENT_64(addr) if (addr % 8 != 0)\
{\
    cpu.raise_exception(Exception::LoadAddressMisaligned);\
    return;\
}

#define CHECK_STORE_ALIGNMENT_32(addr) if (addr % 4 != 0)\
{\
    cpu.raise_exception(Exception::StoreOrAMOAddressMisaligned);\
    return;\
}

#define CHECK_STORE_ALIGNMENT_64(addr) if (addr % 8 != 0)\
{\
    cpu.raise_exception(Exception::StoreOrAMOAddressMisaligned);\
    return;\
}

#define ATTEMPT_LOAD_32()\
const auto value = cpu.read_32(address);\
if (!value)\
{\
    cpu.raise_exception(value.error());\
    return;\
}

#define ATTEMPT_LOAD_64()\
const auto value = cpu.read_64(address);\
if (!value)\
{\
    cpu.raise_exception(value.error());\
    return;\
}

#define ATTEMPT_WRITE_32(value)\
const auto error = cpu.write_32(address, value);\
if (error.has_value())\
{\
    cpu.raise_exception(*error);\
    return;\
}

#define ATTEMPT_WRITE_64(value)\
const auto error = cpu.write_64(address, value);\
if (error.has_value())\
{\
    cpu.raise_exception(*error);\
    return;\
}

bool opcodes_a(CPU& cpu, const Instruction instruction)
{
    const u64 opcode = instruction.get_opcode();
    const u64 funct3 = instruction.get_funct3();
    const u64 funct7 = instruction.get_funct7() >> 2;

    if (opcode != OPCODES_A) return false;

    switch (funct3)
    {
        case OPCODES_A_FUNCT_3:
        {
            switch (funct7)
            {
                case LR_W:      lr_w     (cpu, instruction); break;
                case SC_W:      sc_w     (cpu, instruction); break;
                case AMOSWAP_W: amoswap_w(cpu, instruction); break;
                case AMOADD_W:  amoadd_w (cpu, instruction); break;
                case AMOXOR_W:  amoxor_w (cpu, instruction); break;
                case AMOAND_W:  amoand_w (cpu, instruction); break;
                case AMOOR_W:   amoor_w  (cpu, instruction); break;
                case AMOMIN_W:  amomin_w (cpu, instruction); break;
                case AMOMAX_W:  amomax_w (cpu, instruction); break;
                case AMOMINU_W: amominu_w(cpu, instruction); break;
                case AMOMAXU_W: amomaxu_w(cpu, instruction); break;
                default:        return false;
            }
            break;
        }

        case OPCODES_A_64:
        {
            switch (funct7)
            {
                case LR_D:      lr_d     (cpu, instruction); break;
                case SC_D:      sc_d     (cpu, instruction); break;
                case AMOSWAP_D: amoswap_d(cpu, instruction); break;
                case AMOADD_D:  amoadd_d (cpu, instruction); break;
                case AMOXOR_D:  amoxor_d (cpu, instruction); break;
                case AMOAND_D:  amoand_d (cpu, instruction); break;
                case AMOOR_D:   amoor_d  (cpu, instruction); break;
                case AMOMIN_D:  amomin_d (cpu, instruction); break;
                case AMOMAX_D:  amomax_d (cpu, instruction); break;
                case AMOMINU_D: amominu_d(cpu, instruction); break;
                case AMOMAXU_D: amomaxu_d(cpu, instruction); break;
                default:        return false;
            }
            break;
        }

        default: return false;
    }

    return true;
}

void lr_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();

    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
    cpu.bus.reservations.insert(address);
}

void sc_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_STORE_ALIGNMENT_32(address);

    if (cpu.bus.reservations.contains(address))
    {
        // Attempt store if we had a valid reservation
        ATTEMPT_WRITE_32(cpu.registers[instruction.get_rs2()]);
        cpu.registers[instruction.get_rd()] = 0;
        cpu.bus.reservations.erase(address);
    }
    else cpu.registers[instruction.get_rd()] = 1;
}

void amoswap_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();
    ATTEMPT_WRITE_32(cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
}

void amoadd_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();
    ATTEMPT_WRITE_32(*value + cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
}

void amoxor_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();
    ATTEMPT_WRITE_32(*value ^ cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
}

void amoand_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();
    ATTEMPT_WRITE_32(*value & cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
}

void amoor_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();
    ATTEMPT_WRITE_32(*value | cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
}

void amomin_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();
    ATTEMPT_WRITE_32((i64)(i32)std::min(
        (i32)*value,
        (i32)cpu.registers[instruction.get_rs2()]
    ));
    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
}

void amomax_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();
    ATTEMPT_WRITE_32((i64)(i32)std::max(
        (i32)*value,
        (i32)cpu.registers[instruction.get_rs2()]
    ));
    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
}

void amominu_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();
    ATTEMPT_WRITE_32((i64)(i32)std::min(
        (u32)*value,
        (u32)cpu.registers[instruction.get_rs2()]
    ));

    // Set rd to old value
    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
}

void amomaxu_w(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_32(address);
    ATTEMPT_LOAD_32();
    ATTEMPT_WRITE_32((i64)(i32)std::max(
        (u32)*value,
        (u32)cpu.registers[instruction.get_rs2()]
    ));
    cpu.registers[instruction.get_rd()] = (i64)(i32)*value;
}


void lr_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();

    cpu.registers[instruction.get_rd()] = *value;
    cpu.bus.reservations.insert(address);
}

void sc_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_STORE_ALIGNMENT_64(address);

    if (cpu.bus.reservations.contains(address))
    {
        // Attempt store if we had a valid reservation
        ATTEMPT_WRITE_64(cpu.registers[instruction.get_rs2()]);
        cpu.registers[instruction.get_rd()] = 0;
        cpu.bus.reservations.erase(address);
    }
    else cpu.registers[instruction.get_rd()] = 1;
}

void amoswap_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();
    ATTEMPT_WRITE_64(cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = *value;
}

void amoadd_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();
    ATTEMPT_WRITE_64(*value + cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = *value;
}

void amoxor_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();
    ATTEMPT_WRITE_64(*value ^ cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = *value;
}

void amoand_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();
    ATTEMPT_WRITE_64(*value & cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = *value;
}

void amoor_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();
    ATTEMPT_WRITE_64(*value | cpu.registers[instruction.get_rs2()]);
    cpu.registers[instruction.get_rd()] = *value;
}

void amomin_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();
    ATTEMPT_WRITE_64(std::min(
        (i64)*value,
        (i64)cpu.registers[instruction.get_rs2()]
    ));
    cpu.registers[instruction.get_rd()] = (i64)*value;
}

void amomax_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();
    ATTEMPT_WRITE_64(std::max(
        (i64)*value,
        (i64)cpu.registers[instruction.get_rs2()]
    ));
    cpu.registers[instruction.get_rd()] = (i64)*value;
}

void amominu_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();
    ATTEMPT_WRITE_64(std::min(
        *value,
        cpu.registers[instruction.get_rs2()]
    ));
    cpu.registers[instruction.get_rd()] = *value;
}

void amomaxu_d(CPU& cpu, const Instruction instruction)
{
    GET_ADDRESS();
    CHECK_LOAD_ALIGNMENT_64(address);
    ATTEMPT_LOAD_64();
    ATTEMPT_WRITE_64(std::max(
        *value,
        cpu.registers[instruction.get_rs2()]
    ));
    cpu.registers[instruction.get_rd()] = *value;
}
