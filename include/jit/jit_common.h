#pragma once

#define rs1 load_register(context, context.current_instruction.get_rs1())
#define rs2 load_register(context, context.current_instruction.get_rs2())
#define set_rd(r) store_register(context, context.current_instruction.get_rd(), r)
#define u1_im(x) llvm::ConstantInt::get(context.builder.getInt1Ty(), x)
#define u8_im(x) llvm::ConstantInt::get(context.builder.getInt8Ty(), x)
#define u16_im(x) llvm::ConstantInt::get(context.builder.getInt16Ty(), x)
#define u32_im(x) llvm::ConstantInt::get(context.builder.getInt32Ty(), x)
#define u64_im(x) llvm::ConstantInt::get(context.builder.getInt64Ty(), x)
#define u64_to_32(x) context.builder.CreateTrunc(x, context.builder.getInt32Ty())
#define u64_to_16(x) context.builder.CreateTrunc(x, context.builder.getInt16Ty())
#define u64_to_8(x) context.builder.CreateTrunc(x, context.builder.getInt8Ty())
#define stop_translation() context.return_pc = context.pc + 4

#ifdef JIT_ENABLE_FALLBACK
static void fall_back(llvm::Function* function, JIT::Context& context)
{
    llvm::Value* did_succeed = context.builder.CreateCall(function, {
        u32_im(context.current_instruction.instruction),
        u64_im(context.pc)
    });

    llvm::Function* root = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* success_block = llvm::BasicBlock::Create(context.context);
    llvm::BasicBlock* failure_block = llvm::BasicBlock::Create(context.context);
    context.builder.CreateCondBr(did_succeed, success_block, failure_block);

    // Exception occured so the PC's about to change - abort!
    context.builder.SetInsertPoint(failure_block);
    context.builder.CreateRet(u64_im(0));
    root->insert(root->end(), failure_block);

    // All went well; carry on
    root->insert(root->end(), success_block);
    context.builder.SetInsertPoint(success_block);
}
#endif
