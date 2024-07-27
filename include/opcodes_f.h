#pragma once
#include "cpu.h"
#include "instruction.h"

#define OPCODES_F_1 0x07
#define OPCODES_F_2 0x27
#define OPCODES_F_3 0x43
#define OPCODES_F_4 0x47
#define OPCODES_F_5 0x4b
#define OPCODES_F_6 0x4f
#define OPCODES_F_7 0x53

#define FLW         0x2
#define FADD_S      0x0
#define FSUB_S      0x4
#define FMUL_S      0x8
#define FDIV_S      0xc
#define FMV_X_W     0x0

void init_opcodes_f();

bool opcodes_f(CPU& cpu, const Instruction instruction);

void flw        (CPU& cpu, const Instruction instruction);
void fadd_s     (CPU& cpu, const Instruction instruction);
void fsub_s     (CPU& cpu, const Instruction instruction);
void fmul_s     (CPU& cpu, const Instruction instruction);
void fdiv_s     (CPU& cpu, const Instruction instruction);
void fmv_x_w    (CPU& cpu, const Instruction instruction);