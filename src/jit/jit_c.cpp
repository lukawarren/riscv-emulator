#include "jit/jit_c.h"
#include "jit/jit_common.h"

using namespace JIT;

void JIT::c_lw(Context& context)
{
    set_rd_c_alt(sign_extend(context, perform_load(context, context.on_lw, [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_imm(CompressedInstruction::Type::CL);
        llvm::Value* _rs1 = load_register(
            context,
            context.current_compressed_instruction.get_rs1_alt()
        );
        return context.builder.CreateAdd(
            _rs1,
            u64_im(offset)
        );
    })));
}

void JIT::c_ld(Context& context)
{
    set_rd_c_alt(perform_load(context, context.on_ld, [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_ld_sd_imm();
        llvm::Value* _rs1 = load_register(
            context,
            context.current_compressed_instruction.get_rs1_alt()
        );
        return context.builder.CreateAdd(
            _rs1,
            u64_im(offset)
        );
    }));
}

void JIT::c_lwsp(Context& context)
{
    set_rd_c(sign_extend(context, perform_load(context, context.on_lw, [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_lwsp_offset();
        return context.builder.CreateAdd(
            sp,
            u64_im(offset)
        );
    })));
}

void JIT::c_ldsp(Context& context)
{
    set_rd_c(perform_load(context, context.on_lw, [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_ldsp_offset();
        return context.builder.CreateAdd(
            sp,
            u64_im(offset)
        );
    }));
}

void JIT::c_sw(Context& context)
{

}

void JIT::c_sd(Context& context)
{

}

void JIT::c_swsp(Context& context)
{

}

void JIT::c_sdsp(Context& context)
{

}

void JIT::c_j(Context& context)
{

}

void JIT::c_jr(Context& context)
{

}

void JIT::c_jalr(Context& context)
{

}

void JIT::c_beqz(Context& context)
{

}

void JIT::c_bnez(Context& context)
{

}

void JIT::c_li(Context& context)
{

}

void JIT::c_lui(Context& context)
{

}

void JIT::c_addi(Context& context)
{

}

void JIT::c_addiw(Context& context)
{

}

void JIT::c_addi16sp(Context& context)
{

}

void JIT::c_addi4spn(Context& context)
{

}

void JIT::c_slli(Context& context)
{

}

void JIT::c_srli(Context& context)
{

}

void JIT::c_srai(Context& context)
{

}

void JIT::c_andi(Context& context)
{

}

void JIT::c_mv(Context& context)
{

}

void JIT::c_add(Context& context)
{

}

void JIT::c_addw(Context& context)
{

}

void JIT::c_and(Context& context)
{

}

void JIT::c_or(Context& context)
{

}

void JIT::c_xor(Context& context)
{

}

void JIT::c_sub(Context& context)
{

}

void JIT::c_subw(Context& context)
{

}

void JIT::c_ebreak(Context& context)
{

}

