#pragma once
#include "cpu.h"
#include "instruction.h"
#include "types.h"

#define OPCODES_ZICSR   0b1110011
#define CSRRW           0b001
#define CSRRS           0b010
#define CSRRC           0b011
#define CSRRWI          0b101
#define CSRRSI          0b110
#define CSRRCI          0b111

#define CSR_SATP        0x180
#define CSR_MSTATUS     0x300
#define CSR_MEDELEG     0x302
#define CSR_MIDELEG     0x303
#define CSR_MIE         0x304
#define CSR_MTVEC       0x305
#define CSR_MEPEC       0x341
#define CSR_PMPCFG0     0x3a0
#define CSR_PMPCFG15    0x3af
#define CSR_PMPADDR0    0x3b0
#define CSR_PMPADDR63   0x3ef
#define CSR_MNSTATUS    0x744
#define CSR_MHARTID     0xf14

bool opcodes_zicsr(CPU& cpu, const Instruction& instruction);

CSR read_csr(CPU& cpu, const u16 address);
void write_csr(CPU& cpu, const CSR& csr, const u16 address);

void csrrw(CPU& cpu, const Instruction& instruction);
void csrrs(CPU& cpu, const Instruction& instruction);
void csrrwi(CPU& cpu, const Instruction& instruction);