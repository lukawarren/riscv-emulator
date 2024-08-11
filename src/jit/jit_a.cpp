#include "jit/jit_a.h"
#include "jit/jit_common.h"

using namespace JIT;

#define FALL_BACK()\
    context.builder.CreateCall(context.on_atomic, { u32_im(context.current_instruction.instruction), u64_im(context.pc) });\
    stop_translation();

void JIT::lr_w     (Context& context) { FALL_BACK(); }
void JIT::sc_w     (Context& context) { FALL_BACK(); }
void JIT::amoswap_w(Context& context) { FALL_BACK(); }
void JIT::amoadd_w (Context& context) { FALL_BACK(); }
void JIT::amoxor_w (Context& context) { FALL_BACK(); }
void JIT::amoand_w (Context& context) { FALL_BACK(); }
void JIT::amoor_w  (Context& context) { FALL_BACK(); }
void JIT::amomin_w (Context& context) { FALL_BACK(); }
void JIT::amomax_w (Context& context) { FALL_BACK(); }
void JIT::amominu_w(Context& context) { FALL_BACK(); }
void JIT::amomaxu_w(Context& context) { FALL_BACK(); }

void JIT::lr_d     (Context& context) { FALL_BACK(); }
void JIT::sc_d     (Context& context) { FALL_BACK(); }
void JIT::amoswap_d(Context& context) { FALL_BACK(); }
void JIT::amoadd_d (Context& context) { FALL_BACK(); }
void JIT::amoxor_d (Context& context) { FALL_BACK(); }
void JIT::amoand_d (Context& context) { FALL_BACK(); }
void JIT::amoor_d  (Context& context) { FALL_BACK(); }
void JIT::amomin_d (Context& context) { FALL_BACK(); }
void JIT::amomax_d (Context& context) { FALL_BACK(); }
void JIT::amominu_d(Context& context) { FALL_BACK(); }
void JIT::amomaxu_d(Context& context) { FALL_BACK(); }