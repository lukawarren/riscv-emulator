#pragma once
#include "common.h"
#include "instruction.h"
#include "cpu.h"

#define OPCODES_M           0b0110011
#define OPCODES_M_FUNCT_7   0b1
#define MUL                 0b000
#define MULH                0b001
#define MULHSU              0b010
#define MULHU               0b011
#define DIV                 0b100
#define DIVU                0b101
#define REM                 0b110
#define REMU                0b111

#define OPCODES_M_32        0b0111011
#define MULW                0b000
#define DIVW                0b100
#define DIVUW               0b101
#define REMW                0b110
#define REMUW               0b111

bool opcodes_m(CPU& cpu, const Instruction instruction);

void mul    (CPU& cpu, const Instruction instruction);
void mulh   (CPU& cpu, const Instruction instruction);
void mulhsu (CPU& cpu, const Instruction instruction);
void mulhu  (CPU& cpu, const Instruction instruction);
void div    (CPU& cpu, const Instruction instruction);
void divu   (CPU& cpu, const Instruction instruction);
void rem    (CPU& cpu, const Instruction instruction);
void remu   (CPU& cpu, const Instruction instruction);

void mulw   (CPU& cpu, const Instruction instruction);
void divw   (CPU& cpu, const Instruction instruction);
void divuw  (CPU& cpu, const Instruction instruction);
void remw   (CPU& cpu, const Instruction instruction);
void remuw  (CPU& cpu, const Instruction instruction);
