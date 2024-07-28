#pragma once
#include "instruction.h"

#define OPCODES_F_1 0x07
#define OPCODES_F_2 0x27
#define OPCODES_F_3 0x43
#define OPCODES_F_4 0x47
#define OPCODES_F_5 0x4b
#define OPCODES_F_6 0x4f
#define OPCODES_F_7 0x53

#define FLW         0x2
#define FLD         0x3
#define FSW         0x2
#define FSD         0x3
#define FMADD_S     0x0
#define FMADD_D     0x1
#define FMSUB_S     0x0
#define FMSUB_D     0x1
#define FNMADD_S    0x0
#define FNMADD_D    0x1
#define FNMSUB_S    0x0
#define FNMSUB_D    0x1
#define FADD_S      0x0
#define FADD_D      0x1
#define FSUB_S      0x4
#define FSUB_D      0x5
#define FMUL_S      0x8
#define FMUL_D      0x9
#define FDIV_S      0xc
#define FDIV_D      0xd
#define FSQRT_S     0x2c
#define FSQRT_D     0x2d
#define FSGNJ_S     0x0
#define FSGNJ_D     0x0
#define FSGNJN_S    0x1
#define FSGNJN_D    0x1
#define FSGNJX_S    0x2
#define FSGNJX_D    0x2
#define FMIN_S      0x0
#define FMIN_D      0x0
#define FMAX_S      0x1
#define FMAX_D      0x1
#define FCVT_S_W    0x0
#define FCVT_D_W    0x0
#define FCVT_S_L    0x2
#define FCVT_D_L    0x2
#define FCVT_S_WU   0x1
#define FCVT_D_WU   0x1
#define FCVT_S_LU   0x3
#define FCVT_D_LU   0x3
#define FCVT_S_D    0x20
#define FCVT_D_S    0x21
#define FCVT_D_W    0x0
#define FCVT_W_S    0x0
#define FCVT_W_D    0x0
#define FCVT_L_S    0x2
#define FCVT_L_D    0x2
#define FCVT_WU_S   0x1
#define FCVT_WU_D   0x1
#define FCVT_LU_S   0x3
#define FCVT_LU_D   0x3
#define FMV_X_W     0x0
#define FMV_X_D     0x0
#define FMV_W_X     0x78
#define FMV_D_X     0x79
#define FCLASS_S    0x1
#define FCLASS_D    0x1
#define FEQ_S       0x2
#define FEQ_D       0x2
#define FLT_S       0x1
#define FLT_D       0x1
#define FLE_S       0x0
#define FLE_D       0x0

extern u32 qNaN_float;  // a.k.a. "canconical" NaN
extern u64 qNaN_double; // a.k.a. "canconical" NaN
extern u32 sNaN_float;
extern u64 sNaN_double;

class CPU;

void init_opcodes_f();
bool check_fs_field(CPU& cpu, bool is_write);
bool opcodes_f(CPU& cpu, const Instruction instruction);

void flw        (CPU& cpu, const Instruction instruction);
void fld        (CPU& cpu, const Instruction instruction);
void fsw        (CPU& cpu, const Instruction instruction);
void fsd        (CPU& cpu, const Instruction instruction);
void fmadd_s    (CPU& cpu, const Instruction instruction);
void fmadd_d    (CPU& cpu, const Instruction instruction);
void fmsub_s    (CPU& cpu, const Instruction instruction);
void fmsub_d    (CPU& cpu, const Instruction instruction);
void fnmadd_s   (CPU& cpu, const Instruction instruction);
void fnmadd_d   (CPU& cpu, const Instruction instruction);
void fnmsub_s   (CPU& cpu, const Instruction instruction);
void fnmsub_d   (CPU& cpu, const Instruction instruction);
void fadd_s     (CPU& cpu, const Instruction instruction);
void fadd_d     (CPU& cpu, const Instruction instruction);
void fsub_s     (CPU& cpu, const Instruction instruction);
void fsub_d     (CPU& cpu, const Instruction instruction);
void fmul_s     (CPU& cpu, const Instruction instruction);
void fmul_d     (CPU& cpu, const Instruction instruction);
void fdiv_s     (CPU& cpu, const Instruction instruction);
void fdiv_d     (CPU& cpu, const Instruction instruction);
void fsqrt_s    (CPU& cpu, const Instruction instruction);
void fsqrt_d    (CPU& cpu, const Instruction instruction);
void fsgnj_s    (CPU& cpu, const Instruction instruction);
void fsgnj_d    (CPU& cpu, const Instruction instruction);
void fsgnjn_s   (CPU& cpu, const Instruction instruction);
void fsgnjn_d   (CPU& cpu, const Instruction instruction);
void fsgnjx_s   (CPU& cpu, const Instruction instruction);
void fsgnjx_d   (CPU& cpu, const Instruction instruction);
void fmin_s     (CPU& cpu, const Instruction instruction);
void fmin_d     (CPU& cpu, const Instruction instruction);
void fmax_s     (CPU& cpu, const Instruction instruction);
void fmax_d     (CPU& cpu, const Instruction instruction);
void fcvt_s_w   (CPU& cpu, const Instruction instruction);
void fcvt_s_d   (CPU& cpu, const Instruction instruction);
void fcvt_d_s   (CPU& cpu, const Instruction instruction);
void fcvt_d_w   (CPU& cpu, const Instruction instruction);
void fcvt_s_l   (CPU& cpu, const Instruction instruction);
void fcvt_d_l   (CPU& cpu, const Instruction instruction);
void fcvt_s_wu  (CPU& cpu, const Instruction instruction);
void fcvt_d_wu  (CPU& cpu, const Instruction instruction);
void fcvt_s_lu  (CPU& cpu, const Instruction instruction);
void fcvt_d_lu  (CPU& cpu, const Instruction instruction);
void fcvt_w_s   (CPU& cpu, const Instruction instruction);
void fcvt_w_d   (CPU& cpu, const Instruction instruction);
void fcvt_l_s   (CPU& cpu, const Instruction instruction);
void fcvt_l_d   (CPU& cpu, const Instruction instruction);
void fcvt_wu_s  (CPU& cpu, const Instruction instruction);
void fcvt_wu_d  (CPU& cpu, const Instruction instruction);
void fcvt_lu_s  (CPU& cpu, const Instruction instruction);
void fcvt_lu_d  (CPU& cpu, const Instruction instruction);
void fmv_x_w    (CPU& cpu, const Instruction instruction);
void fmv_x_d    (CPU& cpu, const Instruction instruction);
void fmv_w_x    (CPU& cpu, const Instruction instruction);
void fmv_d_x    (CPU& cpu, const Instruction instruction);
void feq_s      (CPU& cpu, const Instruction instruction);
void feq_d      (CPU& cpu, const Instruction instruction);
void flt_s      (CPU& cpu, const Instruction instruction);
void flt_d      (CPU& cpu, const Instruction instruction);
void fle_s      (CPU& cpu, const Instruction instruction);
void fle_d      (CPU& cpu, const Instruction instruction);
void fclass_s   (CPU& cpu, const Instruction instruction);
void fclass_d   (CPU& cpu, const Instruction instruction);
