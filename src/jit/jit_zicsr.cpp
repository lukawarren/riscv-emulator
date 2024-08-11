#define JIT_ENABLE_FALLBACK
#include "jit/jit_zicsr.h"
#include "jit/jit_common.h"

using namespace JIT;

void JIT::csrrw (Context& context) { fall_back(context.on_csr, context); }
void JIT::csrrc (Context& context) { fall_back(context.on_csr, context); }
void JIT::csrrs (Context& context) { fall_back(context.on_csr, context); }
void JIT::csrrwi(Context& context) { fall_back(context.on_csr, context); }
void JIT::csrrsi(Context& context) { fall_back(context.on_csr, context); }
void JIT::csrrci(Context& context) { fall_back(context.on_csr, context); }
