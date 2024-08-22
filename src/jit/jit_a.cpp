#define JIT_ENABLE_FALLBACK
#include "jit/jit_a.h"
#include "jit/jit_common.h"

using namespace JIT;

void JIT::lr_w     (Context& context) { fall_back(context.on_atomic, context); }
void JIT::sc_w     (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoswap_w(Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoadd_w (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoxor_w (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoand_w (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoor_w  (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amomin_w (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amomax_w (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amominu_w(Context& context) { fall_back(context.on_atomic, context); }
void JIT::amomaxu_w(Context& context) { fall_back(context.on_atomic, context); }

void JIT::lr_d     (Context& context) { fall_back(context.on_atomic, context); }
void JIT::sc_d     (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoswap_d(Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoadd_d (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoxor_d (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoand_d (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amoor_d  (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amomin_d (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amomax_d (Context& context) { fall_back(context.on_atomic, context); }
void JIT::amominu_d(Context& context) { fall_back(context.on_atomic, context); }
void JIT::amomaxu_d(Context& context) { fall_back(context.on_atomic, context); }