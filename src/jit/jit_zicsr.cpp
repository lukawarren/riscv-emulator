#include "jit/jit_zicsr.h"
#include "jit/jit_common.h"

using namespace JIT;

#define FALL_BACK()\
    context.builder.CreateCall(context.on_csr, { u32_im(context.current_instruction.instruction), u64_im(context.pc) });\
    stop_translation();

void JIT::csrrw (Context& context) { FALL_BACK(); }
void JIT::csrrc (Context& context) { FALL_BACK(); }
void JIT::csrrs (Context& context) { FALL_BACK(); }
void JIT::csrrwi(Context& context) { FALL_BACK(); }
void JIT::csrrsi(Context& context) { FALL_BACK(); }
void JIT::csrrci(Context& context) { FALL_BACK(); }
