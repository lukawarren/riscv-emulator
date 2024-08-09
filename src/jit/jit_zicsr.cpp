#include "jit/jit_zicsr.h"
#include "jit/jit_common.h"

using namespace JIT;

#define FALL_BACK()\
    context.builder.CreateCall(context.on_csr, { u32_im(context.current_instruction.instruction), u64_im(context.pc) });\
    stop_translation();

void JIT::csrrw (const Instruction instruction, Context& context) { FALL_BACK(); }
void JIT::csrrc (const Instruction instruction, Context& context) { FALL_BACK(); }
void JIT::csrrs (const Instruction instruction, Context& context) { FALL_BACK(); }
void JIT::csrrwi(const Instruction instruction, Context& context) { FALL_BACK(); }
void JIT::csrrsi(const Instruction instruction, Context& context) { FALL_BACK(); }
void JIT::csrrci(const Instruction instruction, Context& context) { FALL_BACK(); }
