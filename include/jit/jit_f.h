#pragma once
#include "jit/jit.h"

namespace JIT
{
    void flw        (Context& context);
    void fld        (Context& context);
    void fsw        (Context& context);
    void fsd        (Context& context);
    void fmadd_s    (Context& context);
    void fmadd_d    (Context& context);
    void fmsub_s    (Context& context);
    void fmsub_d    (Context& context);
    void fnmadd_s   (Context& context);
    void fnmadd_d   (Context& context);
    void fnmsub_s   (Context& context);
    void fnmsub_d   (Context& context);
    void fadd_s     (Context& context);
    void fadd_d     (Context& context);
    void fsub_s     (Context& context);
    void fsub_d     (Context& context);
    void fmul_s     (Context& context);
    void fmul_d     (Context& context);
    void fdiv_s     (Context& context);
    void fdiv_d     (Context& context);
    void fsqrt_s    (Context& context);
    void fsqrt_d    (Context& context);
    void fsgnj_s    (Context& context);
    void fsgnj_d    (Context& context);
    void fsgnjn_s   (Context& context);
    void fsgnjn_d   (Context& context);
    void fsgnjx_s   (Context& context);
    void fsgnjx_d   (Context& context);
    void fmin_s     (Context& context);
    void fmin_d     (Context& context);
    void fmax_s     (Context& context);
    void fmax_d     (Context& context);
    void fcvt_s_w   (Context& context);
    void fcvt_s_d   (Context& context);
    void fcvt_d_s   (Context& context);
    void fcvt_d_w   (Context& context);
    void fcvt_s_l   (Context& context);
    void fcvt_d_l   (Context& context);
    void fcvt_s_wu  (Context& context);
    void fcvt_d_wu  (Context& context);
    void fcvt_s_lu  (Context& context);
    void fcvt_d_lu  (Context& context);
    void fcvt_w_s   (Context& context);
    void fcvt_w_d   (Context& context);
    void fcvt_l_s   (Context& context);
    void fcvt_l_d   (Context& context);
    void fcvt_wu_s  (Context& context);
    void fcvt_wu_d  (Context& context);
    void fcvt_lu_s  (Context& context);
    void fcvt_lu_d  (Context& context);
    void fmv_x_w    (Context& context);
    void fmv_x_d    (Context& context);
    void fmv_w_x    (Context& context);
    void fmv_d_x    (Context& context);
    void feq_s      (Context& context);
    void feq_d      (Context& context);
    void flt_s      (Context& context);
    void flt_d      (Context& context);
    void fle_s      (Context& context);
    void fle_d      (Context& context);
    void fclass_s   (Context& context);
    void fclass_d   (Context& context);

    void c_fldsp    (Context& context);
    void c_fsdsp    (Context& context);
    void c_fld      (Context& context);
    void c_fsd      (Context& context);
}