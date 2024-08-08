#include "jit/jit_zicsr.h"
#include "jit/jit_common.h"

using namespace JIT;

void JIT::csrrw(const Instruction instruction, Context& context)
{
    context.builder.CreateCall(context.write_to_csr, { u64_im(1), u64_im(2) });
}

void JIT::csrrc(const Instruction instruction, Context& context)
{
    context.builder.CreateCall(context.write_to_csr, { u64_im(1), u64_im(2) });
}

void JIT::csrrs(const Instruction instruction, Context& context)
{
    context.builder.CreateCall(context.write_to_csr, { u64_im(1), u64_im(2) });
}

void JIT::csrrwi(const Instruction instruction, Context& context)
{
    context.builder.CreateCall(context.write_to_csr, { u64_im(1), u64_im(2) });
}

void JIT::csrrsi(const Instruction instruction, Context& context)
{
    context.builder.CreateCall(context.write_to_csr, { u64_im(1), u64_im(2) });
}

void JIT::csrrci(const Instruction instruction, Context& context)
{
    context.builder.CreateCall(context.write_to_csr, { u64_im(1), u64_im(2) });
}
