#pragma once
#include "cpu.h"
#include "instruction.h"

/*
    https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
*/

#define OPCODES_BASE_I_TYPE 0x13
#define ADDI    0x0
#define SLLI    0x1
#define SLTI    0x2
#define SLTIU   0x3
#define XORI    0x4
#define SRI     0x5
#define SRLI    0x00
#define SRAI    0x20
#define ORI     0x6
#define ANDI    0x7

#define JAL     0b1101111
#define JALR    0b1100111

bool opcodes_base(CPU& cpu, const Instruction& instruction);

void addi   (CPU& cpu, const Instruction& instruction);
void slli   (CPU& cpu, const Instruction& instruction);
void slti   (CPU& cpu, const Instruction& instruction);
void sltiu  (CPU& cpu, const Instruction& instruction);
void xori   (CPU& cpu, const Instruction& instruction);
void sri    (CPU& cpu, const Instruction& instruction);
void srli   (CPU& cpu, const Instruction& instruction);
void srai   (CPU& cpu, const Instruction& instruction);
void ori    (CPU& cpu, const Instruction& instruction);
void andi   (CPU& cpu, const Instruction& instruction);

void jal    (CPU& cpu, const Instruction& instruction);