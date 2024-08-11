#include "jit/jit_m.h"
#include "jit/jit_common.h"

using namespace JIT;

void JIT::mul(Context& context)
{
    set_rd(context.builder.CreateMul(rs1, rs2));
}

void JIT::mulh(Context& context)
{
    llvm::Value* _rs1 = context.builder.CreateSExt(rs1, llvm::Type::getInt128Ty(context.context));
    llvm::Value* _rs2 = context.builder.CreateSExt(rs2, llvm::Type::getInt128Ty(context.context));
    set_rd(context.builder.CreateLShr(
        context.builder.CreateMul(_rs1, _rs2),
        64
    ));
}

void JIT::mulhsu(Context& context)
{
    llvm::Value* _rs1 = context.builder.CreateSExt(rs1, llvm::Type::getInt128Ty(context.context));
    llvm::Value* _rs2 = context.builder.CreateZExt(rs2, llvm::Type::getInt128Ty(context.context));
    set_rd(context.builder.CreateLShr(
        context.builder.CreateMul(_rs1, _rs2),
        64
    ));
}

void JIT::mulhu(Context& context)
{
    llvm::Value* _rs1 = context.builder.CreateZExt(rs1, llvm::Type::getInt128Ty(context.context));
    llvm::Value* _rs2 = context.builder.CreateZExt(rs2, llvm::Type::getInt128Ty(context.context));
    set_rd(context.builder.CreateLShr(
        context.builder.CreateMul(_rs1, _rs2),
        64
    ));
}

void JIT::div(Context& context)
{
    llvm::Value* dividend = rs1;
    llvm::Value* divisor = rs2;

    llvm::Value* divisor_is_zero = context.builder.CreateICmpEQ(divisor, u64_im(0));

    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* divide_by_zero = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* check_overflow = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(divisor_is_zero, divide_by_zero, check_overflow);

    // if(divisor == 0)
    context.builder.SetInsertPoint(divide_by_zero);
    set_rd(u64_im(std::numeric_limits<u64>::max()));
    context.builder.CreateCall(context.set_fcsr_dz);
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(check_overflow);

    // Check for overflow
    llvm::Value* is_overflow_a = context.builder.CreateICmpEQ(
        dividend,
        u64_im(std::numeric_limits<i64>::min())
    );
    llvm::Value* is_overflow_b = context.builder.CreateICmpEQ(
        divisor,
        u64_im(-1)
    );
    llvm::Value* is_overflow = context.builder.CreateAnd(is_overflow_a, is_overflow_b);

    llvm::BasicBlock* overflow = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(is_overflow, overflow, normal);

    // if(overflow)
    context.builder.SetInsertPoint(overflow);
    set_rd(dividend);
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(normal);
    set_rd(context.builder.CreateSDiv(dividend, divisor));
    context.builder.CreateBr(done);

    // return block
    context.builder.SetInsertPoint(done);
}

void JIT::divu(Context& context)
{
    llvm::Value* dividend = rs1;
    llvm::Value* divisor = rs2;

    llvm::Value* divisor_is_zero = context.builder.CreateICmpEQ(divisor, u64_im(0));

    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* divide_by_zero = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(divisor_is_zero, divide_by_zero, normal);

    // if(divisor == 0)
    context.builder.SetInsertPoint(divide_by_zero);
    set_rd(u64_im(std::numeric_limits<u64>::max()));
    context.builder.CreateCall(context.set_fcsr_dz);
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(normal);
    set_rd(context.builder.CreateUDiv(dividend, divisor));
    context.builder.CreateBr(done);

    // return block
    context.builder.SetInsertPoint(done);
}

void JIT::rem(Context& context)
{
    llvm::Value* dividend = rs1;
    llvm::Value* divisor = rs2;

    llvm::Value* divisor_is_zero = context.builder.CreateICmpEQ(divisor, u64_im(0));

    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* divide_by_zero = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* check_overflow = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(divisor_is_zero, divide_by_zero, check_overflow);

    // if(divisor == 0)
    context.builder.SetInsertPoint(divide_by_zero);
    set_rd(dividend);
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(check_overflow);

    // Check for overflow
    llvm::Value* is_overflow_a = context.builder.CreateICmpEQ(
        dividend,
        u64_im(std::numeric_limits<i64>::min())
    );
    llvm::Value* is_overflow_b = context.builder.CreateICmpEQ(
        divisor,
        u64_im(-1)
    );
    llvm::Value* is_overflow = context.builder.CreateAnd(is_overflow_a, is_overflow_b);

    llvm::BasicBlock* overflow = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(is_overflow, overflow, normal);

    // if(overflow)
    context.builder.SetInsertPoint(overflow);
    set_rd(u64_im(0));
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(normal);
    set_rd(context.builder.CreateSRem(dividend, divisor));
    context.builder.CreateBr(done);

    // return block
    context.builder.SetInsertPoint(done);
}

void JIT::remu(Context& context)
{
    llvm::Value* dividend = rs1;
    llvm::Value* divisor = rs2;

    llvm::Value* divisor_is_zero = context.builder.CreateICmpEQ(divisor, u64_im(0));

    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* divide_by_zero = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(divisor_is_zero, divide_by_zero, normal);

    // if(divisor == 0)
    context.builder.SetInsertPoint(divide_by_zero);
    set_rd(dividend);
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(normal);
    set_rd(context.builder.CreateURem(dividend, divisor));
    context.builder.CreateBr(done);

    // return block
    context.builder.SetInsertPoint(done);
}

void JIT::mulw(Context& context)
{
    llvm::Value* _rs1 = context.builder.CreateTrunc(rs1, context.builder.getInt32Ty());
    llvm::Value* _rs2 = context.builder.CreateTrunc(rs2, context.builder.getInt32Ty());
    set_rd(
        sign_extend(
            context,
            context.builder.CreateMul(_rs1, _rs2)
        )
    );
}

void JIT::divw(Context& context)
{
    llvm::Value* dividend = context.builder.CreateTrunc(rs1, context.builder.getInt32Ty());
    llvm::Value* divisor = context.builder.CreateTrunc(rs2, context.builder.getInt32Ty());

    llvm::Value* divisor_is_zero = context.builder.CreateICmpEQ(divisor, u32_im(0));

    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* divide_by_zero = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* check_overflow = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(divisor_is_zero, divide_by_zero, check_overflow);

    // if(divisor == 0)
    context.builder.SetInsertPoint(divide_by_zero);
    set_rd(u64_im(std::numeric_limits<u64>::max()));
    context.builder.CreateCall(context.set_fcsr_dz);
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(check_overflow);

    // Check for overflow
    llvm::Value* is_overflow_a = context.builder.CreateICmpEQ(
        dividend,
        u32_im(std::numeric_limits<i32>::min())
    );
    llvm::Value* is_overflow_b = context.builder.CreateICmpEQ(
        divisor,
        u32_im(-1)
    );
    llvm::Value* is_overflow = context.builder.CreateAnd(is_overflow_a, is_overflow_b);

    llvm::BasicBlock* overflow = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(is_overflow, overflow, normal);

    // if(overflow)
    context.builder.SetInsertPoint(overflow);
    set_rd(
        sign_extend(
            context,
            dividend
        )
    );
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(normal);
    set_rd(
        sign_extend(
            context,
            context.builder.CreateSDiv(dividend, divisor)
        )
    );
    context.builder.CreateBr(done);

    // return block
    context.builder.SetInsertPoint(done);
}

void JIT::divuw(Context& context)
{
    llvm::Value* dividend = context.builder.CreateTrunc(rs1, context.builder.getInt32Ty());
    llvm::Value* divisor = context.builder.CreateTrunc(rs2, context.builder.getInt32Ty());

    llvm::Value* divisor_is_zero = context.builder.CreateICmpEQ(divisor, u32_im(0));

    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* divide_by_zero = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(divisor_is_zero, divide_by_zero, normal);

    // if(divisor == 0)
    context.builder.SetInsertPoint(divide_by_zero);
    set_rd(u64_im(std::numeric_limits<u64>::max()));
    context.builder.CreateCall(context.set_fcsr_dz);
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(normal);
    set_rd(
        sign_extend(
            context,
            context.builder.CreateUDiv(dividend, divisor)
        )
    );
    context.builder.CreateBr(done);

    // return block
    context.builder.SetInsertPoint(done);
}

void JIT::remw(Context& context)
{
    llvm::Value* dividend = context.builder.CreateTrunc(rs1, context.builder.getInt32Ty());
    llvm::Value* divisor = context.builder.CreateTrunc(rs2, context.builder.getInt32Ty());

    llvm::Value* divisor_is_zero = context.builder.CreateICmpEQ(divisor, u32_im(0));

    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* divide_by_zero = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* check_overflow = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(divisor_is_zero, divide_by_zero, check_overflow);

    // if(divisor == 0)
    context.builder.SetInsertPoint(divide_by_zero);
    set_rd(
        sign_extend(
            context,
            dividend
        )
    );
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(check_overflow);

    // Check for overflow
    llvm::Value* is_overflow_a = context.builder.CreateICmpEQ(
        dividend,
        u32_im(std::numeric_limits<i32>::min())
    );
    llvm::Value* is_overflow_b = context.builder.CreateICmpEQ(
        divisor,
        u32_im(-1)
    );
    llvm::Value* is_overflow = context.builder.CreateAnd(is_overflow_a, is_overflow_b);

    llvm::BasicBlock* overflow = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(is_overflow, overflow, normal);

    // if(overflow)
    context.builder.SetInsertPoint(overflow);
    set_rd(u64_im(0));
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(normal);
    set_rd(
        sign_extend(
            context,
            context.builder.CreateSRem(dividend, divisor)
        )
    );
    context.builder.CreateBr(done);

    // return block
    context.builder.SetInsertPoint(done);
}

void JIT::remuw(Context& context)
{
    llvm::Value* dividend = context.builder.CreateTrunc(rs1, context.builder.getInt32Ty());
    llvm::Value* divisor = context.builder.CreateTrunc(rs2, context.builder.getInt32Ty());

    llvm::Value* divisor_is_zero = context.builder.CreateICmpEQ(divisor, u32_im(0));

    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* divide_by_zero = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context.context, "", function);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context.context, "", function);
    context.builder.CreateCondBr(divisor_is_zero, divide_by_zero, normal);

    // if(divisor == 0)
    context.builder.SetInsertPoint(divide_by_zero);
    set_rd(
        sign_extend(
            context,
            dividend
        )
    );
    context.builder.CreateBr(done);

    // else
    context.builder.SetInsertPoint(normal);
    set_rd(
        sign_extend(
            context,
            context.builder.CreateURem(dividend, divisor)
        )
    );
    context.builder.CreateBr(done);

    // return block
    context.builder.SetInsertPoint(done);
}
