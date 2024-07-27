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
#define FSW         0x2
#define FMADD_S     0x0
#define FMSUB_S     0x0
#define FNMADD_S    0x0
#define FNMSUB_S    0x0
#define FADD_S      0x0
#define FSUB_S      0x4
#define FMUL_S      0x8
#define FDIV_S      0xc
#define FDIV_SQRT_S 0x2c
#define FSGNJ_S     0x0
#define FSGNJN_S    0x1
#define FSGNJX_S    0x2
#define FMIN_S      0x0
#define FMAX_S      0x1
#define FCVT_S_W    0x0
#define FCVT_S_L    0x2
#define FCVT_S_WU   0x1
#define FCVT_S_LU   0x3
#define FCVT_W_S    0x0
#define FCVT_L_S    0x2
#define FCVT_WU_S   0x1
#define FCVT_LU_S   0x3
#define FMV_X_W     0x0
#define FMV_W_X     0x78
#define FCLASS_S    0x1
#define FEQ_S       0x2
#define FLT_S       0x1
#define FLE_S       0x0

void init_opcodes_f();

bool opcodes_f(CPU& cpu, const Instruction instruction);

void flw        (CPU& cpu, const Instruction instruction);
void fsw        (CPU& cpu, const Instruction instruction);
void fmadd_s    (CPU& cpu, const Instruction instruction);
void fmsub_s    (CPU& cpu, const Instruction instruction);
void fnmadd_s   (CPU& cpu, const Instruction instruction);
void fnmsub_s   (CPU& cpu, const Instruction instruction);
void fadd_s     (CPU& cpu, const Instruction instruction);
void fsub_s     (CPU& cpu, const Instruction instruction);
void fmul_s     (CPU& cpu, const Instruction instruction);
void fdiv_s     (CPU& cpu, const Instruction instruction);
void fsqrt_s    (CPU& cpu, const Instruction instruction);
void fsgnj_s    (CPU& cpu, const Instruction instruction);
void fsgnjn_s   (CPU& cpu, const Instruction instruction);
void fsgnjx_s   (CPU& cpu, const Instruction instruction);
void fmin_s     (CPU& cpu, const Instruction instruction);
void fmax_s     (CPU& cpu, const Instruction instruction);
void fcvt_s_w   (CPU& cpu, const Instruction instruction);
void fcvt_s_l   (CPU& cpu, const Instruction instruction);
void fcvt_s_wu  (CPU& cpu, const Instruction instruction);
void fcvt_s_lu  (CPU& cpu, const Instruction instruction);
void fcvt_w_s   (CPU& cpu, const Instruction instruction);
void fcvt_l_s   (CPU& cpu, const Instruction instruction);
void fcvt_wu_s  (CPU& cpu, const Instruction instruction);
void fcvt_lu_s  (CPU& cpu, const Instruction instruction);
void fmv_x_w    (CPU& cpu, const Instruction instruction);
void fmv_w_x    (CPU& cpu, const Instruction instruction);
void feq_s      (CPU& cpu, const Instruction instruction);
void flt_s      (CPU& cpu, const Instruction instruction);
void fle_s      (CPU& cpu, const Instruction instruction);
void fclass_s   (CPU& cpu, const Instruction instruction);
