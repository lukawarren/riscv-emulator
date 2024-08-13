#include "jit/jit_c.h"
#include "jit/jit_common.h"

using namespace JIT;

static void emit_compressed_branch(Context& context, llvm::Value* condition);

void JIT::c_lw(Context& context)
{
    set_rd_c_alt(sign_extend(context, perform_load(context, context.on_lw, [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_imm(CompressedInstruction::Type::CL);
        return context.builder.CreateAdd(
            rs1_c_alt,
            u64_im(offset)
        );
    })));
}

void JIT::c_ld(Context& context)
{
    set_rd_c_alt(perform_load(context, context.on_ld, [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_ld_sd_imm();
        return context.builder.CreateAdd(
            rs1_c_alt,
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
    set_rd_c(perform_load(context, context.on_ld, [](Context& context)
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
    perform_store(context, context.on_sw, u64_to_32(rs2_c_alt), [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_imm(CompressedInstruction::Type::CL);
        return context.builder.CreateAdd(
            rs1_c_alt,
            u64_im(offset)
        );
    });
}

void JIT::c_sd(Context& context)
{
    perform_store(context, context.on_sd, rs2_c_alt, [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_ld_sd_imm();
        return context.builder.CreateAdd(
            rs1_c_alt,
            u64_im(offset)
        );
    });
}

void JIT::c_swsp(Context& context)
{
    perform_store(context, context.on_sw, u64_to_32(rs2_c), [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_swsp_offset();
        return context.builder.CreateAdd(
            sp,
            u64_im(offset)
        );
    });
}

void JIT::c_sdsp(Context& context)
{
    perform_store(context, context.on_sd, rs2_c, [](Context& context)
    {
        const auto offset = context.current_compressed_instruction.get_sdsp_offset();
        return context.builder.CreateAdd(
            sp,
            u64_im(offset)
        );
    });
}

void JIT::c_j(Context& context)
{
    context.pc += context.current_compressed_instruction.get_jump_offset() - 2;
}

void JIT::c_jr(Context& context)
{
    const auto _rs1 = context.current_compressed_instruction.get_rs1();
    if (_rs1 != 0)
    {
        create_non_terminating_return(context, load_register(context, _rs1));
    }
}

void JIT::c_jalr(Context& context)
{
    store_register(context, 1, u64_im(context.pc + 2));
    create_non_terminating_return(context, rs1_c);
}

void JIT::c_beqz(Context& context)
{
    emit_compressed_branch(context, context.builder.CreateICmpEQ(rs1_c_alt, u64_im(0)));
}

void JIT::c_bnez(Context& context)
{
    emit_compressed_branch(context, context.builder.CreateICmpNE(rs1_c_alt, u64_im(0)));
}

void JIT::c_li(Context& context)
{
    set_rd_c(u64_im(context.current_compressed_instruction.get_none_zero_imm()));
}

void JIT::c_lui(Context& context)
{
    set_rd_c(u64_im(context.current_compressed_instruction.get_lui_non_zero_imm()));
}

void JIT::c_addi(Context& context)
{
    const auto rd = context.current_compressed_instruction.get_rd();
    llvm::Value* result = context.builder.CreateAdd(
        load_register(context, rd),
        u64_im(context.current_compressed_instruction.get_none_zero_imm())
    );
    store_register(context, rd, result);
}

void JIT::c_addiw(Context& context)
{
    // As with c.addi, but take the lower 32 bits, then sign extend to 64
    const auto rd = context.current_compressed_instruction.get_rd();
    llvm::Value* result = context.builder.CreateAdd(
        load_register(context, rd),
        u64_im(context.current_compressed_instruction.get_none_zero_imm())
    );
    store_register(context, rd, sign_extend_32_as_64(context, result));
}

void JIT::c_addi16sp(Context& context)
{
    llvm::Value* result = context.builder.CreateAdd(
        sp,
        u64_im(context.current_compressed_instruction.get_addi16sp_none_zero_imm())
    );
    store_register(context, 2, result);
}

void JIT::c_addi4spn(Context& context)
{
    const u64 imm = context.current_compressed_instruction.get_addi4spn_none_zero_unsigned_imm();
    set_rd_c_alt(context.builder.CreateAdd(sp, u64_im(imm)));
}

void JIT::c_slli(Context& context)
{
    const u8 _rd = context.current_compressed_instruction.get_rd();
    const auto shamt = context.current_compressed_instruction.get_shamt();
    store_register(context, _rd, context.builder.CreateShl(
        load_register(context, _rd),
        shamt
    ));
}

void JIT::c_srli(Context& context)
{
    const u8 _rd = context.current_compressed_instruction.get_rd_with_offset();
    const auto shamt = context.current_compressed_instruction.get_shamt();
    store_register(context, _rd, context.builder.CreateLShr(
        load_register(context, _rd),
        shamt
    ));
}

void JIT::c_srai(Context& context)
{
    const u8 _rd = context.current_compressed_instruction.get_rd_with_offset();
    const auto shamt = context.current_compressed_instruction.get_shamt();
    store_register(context, _rd, context.builder.CreateAShr(
        load_register(context, _rd),
        shamt
    ));
}

void JIT::c_andi(Context& context)
{
    const u8 _rd = context.current_compressed_instruction.get_rd_with_offset();
    store_register(context, _rd, context.builder.CreateAnd(
        load_register(context, _rd),
        context.current_compressed_instruction.get_none_zero_imm()
    ));
}

void JIT::c_mv(Context& context)
{
    set_rd_c(rs2_c);
}

void JIT::c_add(Context& context)
{
    const u8 _rd = context.current_compressed_instruction.get_rd();
    store_register(context, _rd, context.builder.CreateAdd(
        load_register(context, _rd),
        rs2_c
    ));
}

void JIT::c_addw(Context& context)
{
    // As with c.addw, but take the lower 32 bits, then sign extend to 64
    const u8 _rd = context.current_compressed_instruction.get_rs1_alt();
    llvm::Value* result = context.builder.CreateAdd(
        load_register(context, _rd),
        rs2_c_alt
    );
    store_register(context, _rd, sign_extend_32_as_64(context, result));
}

void JIT::c_and(Context& context)
{
    const u8 _rd = context.current_compressed_instruction.get_rd_with_offset();
    store_register(context, _rd, context.builder.CreateAnd(
        load_register(context, _rd),
        rs2_c_alt
    ));
}

void JIT::c_or(Context& context)
{
    const u8 _rd = context.current_compressed_instruction.get_rd_with_offset();
    store_register(context, _rd, context.builder.CreateOr(
        load_register(context, _rd),
        rs2_c_alt
    ));
}

void JIT::c_xor(Context& context)
{
    const u8 _rd = context.current_compressed_instruction.get_rd_with_offset();
    store_register(context, _rd, context.builder.CreateXor(
        load_register(context, _rd),
        rs2_c_alt
    ));
}

void JIT::c_sub(Context& context)
{
    const u8 _rd = context.current_compressed_instruction.get_rd_with_offset();
    store_register(context, _rd, context.builder.CreateSub(
        load_register(context, _rd),
        rs2_c_alt
    ));
}

void JIT::c_subw(Context& context)
{
    // As with c.sub, but take the lower 32 bits, then sign extend to 64
    const u8 _rd = context.current_compressed_instruction.get_rd_with_offset();
    llvm::Value* result = context.builder.CreateSub(
        load_register(context, _rd),
        rs2_c_alt
    );
    store_register(context, _rd, sign_extend_32_as_64(context, result));
}

void JIT::c_ebreak(Context& context)
{
    call_handler_and_return(context, context.on_ebreak);
}

// -- Helpers --

static void emit_compressed_branch(Context& context, llvm::Value* condition)
{
    // Check branch alignment
    const u64 target = context.current_compressed_instruction.get_branch_offset();
    if ((target & 0b1) != 0)
        std::runtime_error("todo: raise exception on unaligned jump");

    llvm::Function* function = context.builder.GetInsertBlock()->getParent();

    // Create blocks
    llvm::BasicBlock* true_block = llvm::BasicBlock::Create(context.context);
    llvm::BasicBlock* false_block = llvm::BasicBlock::Create(context.context);
    context.builder.CreateCondBr(condition, true_block, false_block);

    // True block - unable to JIT further (for now!) so return early
    context.builder.SetInsertPoint(true_block);
    context.builder.CreateRet(u64_im(context.pc + target));
    function->insert(function->end(), true_block);

    // False block - merge back
    function->insert(function->end(), false_block);
    context.builder.SetInsertPoint(false_block);
}
