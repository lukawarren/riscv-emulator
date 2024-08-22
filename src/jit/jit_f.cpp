#define JIT_ENABLE_FALLBACK
#include "jit/jit_f.h"
#include "jit/jit_common.h"

using namespace JIT;

void JIT::flw        (Context& context) { fall_back(context.on_floating, context); }
void JIT::fld        (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsw        (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsd        (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmadd_s    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmadd_d    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmsub_s    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmsub_d    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fnmadd_s   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fnmadd_d   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fnmsub_s   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fnmsub_d   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fadd_s     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fadd_d     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsub_s     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsub_d     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmul_s     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmul_d     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fdiv_s     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fdiv_d     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsqrt_s    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsqrt_d    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsgnj_s    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsgnj_d    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsgnjn_s   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsgnjn_d   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsgnjx_s   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fsgnjx_d   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmin_s     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmin_d     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmax_s     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmax_d     (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_s_w   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_s_d   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_d_s   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_d_w   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_s_l   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_d_l   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_s_wu  (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_d_wu  (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_s_lu  (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_d_lu  (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_w_s   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_w_d   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_l_s   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_l_d   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_wu_s  (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_wu_d  (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_lu_s  (Context& context) { fall_back(context.on_floating, context); }
void JIT::fcvt_lu_d  (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmv_x_w    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmv_x_d    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmv_w_x    (Context& context) { fall_back(context.on_floating, context); }
void JIT::fmv_d_x    (Context& context) { fall_back(context.on_floating, context); }
void JIT::feq_s      (Context& context) { fall_back(context.on_floating, context); }
void JIT::feq_d      (Context& context) { fall_back(context.on_floating, context); }
void JIT::flt_s      (Context& context) { fall_back(context.on_floating, context); }
void JIT::flt_d      (Context& context) { fall_back(context.on_floating, context); }
void JIT::fle_s      (Context& context) { fall_back(context.on_floating, context); }
void JIT::fle_d      (Context& context) { fall_back(context.on_floating, context); }
void JIT::fclass_s   (Context& context) { fall_back(context.on_floating, context); }
void JIT::fclass_d   (Context& context) { fall_back(context.on_floating, context); }

void JIT::c_fldsp    (Context& context) { fall_back(context.on_floating_compressed, context, true); }
void JIT::c_fsdsp    (Context& context) { fall_back(context.on_floating_compressed, context, true); }
void JIT::c_fld      (Context& context) { fall_back(context.on_floating_compressed, context, true); }
void JIT::c_fsd      (Context& context) { fall_back(context.on_floating_compressed, context, true); }
