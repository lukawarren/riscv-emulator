#pragma once
#include "cpu.h"
#include "instruction.h"

/*
    https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
*/

#define OPCODES_BASE_I_TYPE 0x13
#define ADDI    0x0
#define XORI    0x4
#define ORI     0x6
#define ANDI    0x7
#define SLLI    0x1
#define SRLI    0x00
#define SRAI    0x20
#define SLTI    0x2
#define SLTIU   0x3

#define OPCODES_BASE_B_TYPE 0x63
#define BEQ     0x0
#define BNE     0x1
#define BLT     0x4
#define BGE     0x5
#define BLTU    0x6
#define BGEU    0x7

#define JAL     0b1101111
#define JALR    0b1100111

#define LUI     0b0110111
#define AUIPC   0b0010111

// --- RV64I-specific ---
#define OPCODES_BASE_I_TYPE_32 0b0011011
#define ADDIW   0b000

bool opcodes_base(CPU& cpu, const Instruction& instruction);

void addi   (CPU& cpu, const Instruction& instruction);
void xori   (CPU& cpu, const Instruction& instruction);
void ori    (CPU& cpu, const Instruction& instruction);
void andi   (CPU& cpu, const Instruction& instruction);
void slli   (CPU& cpu, const Instruction& instruction);
void srli   (CPU& cpu, const Instruction& instruction);
void srai   (CPU& cpu, const Instruction& instruction);
void slti   (CPU& cpu, const Instruction& instruction);
void sltiu  (CPU& cpu, const Instruction& instruction);

void beq    (CPU& cpu, const Instruction& instruction);
void bne    (CPU& cpu, const Instruction& instruction);
void blt    (CPU& cpu, const Instruction& instruction);
void bge    (CPU& cpu, const Instruction& instruction);
void bltu   (CPU& cpu, const Instruction& instruction);
void bgeu   (CPU& cpu, const Instruction& instruction);

void jal    (CPU& cpu, const Instruction& instruction);

void lui    (CPU& cpu, const Instruction& instruction);
void auipc  (CPU& cpu, const Instruction& instruction);

void addiw  (CPU& cpu, const Instruction& instruction);