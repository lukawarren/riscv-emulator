#pragma once

#define rs1 load_register(context, context.current_instruction.get_rs1())
#define rs1_c load_register(context, context.current_compressed_instruction.get_rs1())
#define rs1_c_alt load_register(context, context.current_compressed_instruction.get_rs1_alt())
#define rs2 load_register(context, context.current_instruction.get_rs2())
#define rs2_c load_register(context, context.current_compressed_instruction.get_rs2())
#define rs2_c_alt load_register(context, context.current_compressed_instruction.get_rs2_alt())
#define sp load_register(context, 2)
#define set_rd(r) store_register(context, context.current_instruction.get_rd(), r)
#define set_rd_c(r) store_register(context, context.current_compressed_instruction.get_rd(), r)
#define set_rd_c_alt(r) store_register(context, context.current_compressed_instruction.get_rd_alt(), r)
#define u1_im(x) llvm::ConstantInt::get(context.builder.getInt1Ty(), x)
#define u8_im(x) llvm::ConstantInt::get(context.builder.getInt8Ty(), x)
#define u16_im(x) llvm::ConstantInt::get(context.builder.getInt16Ty(), x)
#define u32_im(x) llvm::ConstantInt::get(context.builder.getInt32Ty(), x)
#define u64_im(x) llvm::ConstantInt::get(context.builder.getInt64Ty(), x)
#define u64_to_32(x) context.builder.CreateTrunc(x, context.builder.getInt32Ty())
#define u64_to_16(x) context.builder.CreateTrunc(x, context.builder.getInt16Ty())
#define u64_to_8(x) context.builder.CreateTrunc(x, context.builder.getInt8Ty())
#define abort_translation() context.abort_translation = true

#ifdef JIT_ENABLE_FALLBACK
static void fall_back(llvm::Function* function, JIT::Context& context, bool is_compressed = false)
{
    llvm::Value* did_succeed = context.builder.CreateCall(function, {
        is_compressed ?
            u16_im(context.current_compressed_instruction.instruction) :
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

inline llvm::Value* sign_extend_32_as_64(JIT::Context& context, llvm::Value* value)
{
    return context.builder.CreateSExt(
        u64_to_32(value),
        context.builder.getInt64Ty()
    );
}

inline llvm::Value* sign_extend(JIT::Context& context, llvm::Value* value)
{
    return context.builder.CreateSExt(
        value,
        context.builder.getInt64Ty()
    );
}

inline llvm::Value* zero_extend(JIT::Context& context, llvm::Value* value)
{
    return context.builder.CreateZExt(
        value,
        context.builder.getInt64Ty()
    );
}

template<typename F>
llvm::Value* perform_load(JIT::Context& context, llvm::Function* f, F&& get_address)
{
    const auto bool_type = llvm::Type::getInt1Ty(context.context);
    llvm::Value* did_succeed_ptr = context.builder.CreateAlloca(bool_type);
    llvm::Value* result = context.builder.CreateCall(f, {
        get_address(context),
        u64_im(context.pc),
        did_succeed_ptr
    });
    llvm::Value* did_succeed = context.builder.CreateLoad(bool_type, did_succeed_ptr);

    // Return early on failure
    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* success_block = llvm::BasicBlock::Create(context.context);
    llvm::BasicBlock* failure_block = llvm::BasicBlock::Create(context.context);
    context.builder.CreateCondBr(did_succeed, success_block, failure_block);

    // Failure block - unable to JIT further (for now!) so return early
    context.builder.SetInsertPoint(failure_block);
    context.builder.CreateRet(u64_im(0));
    function->insert(function->end(), failure_block);

    // Success block - carry on
    function->insert(function->end(), success_block);
    context.builder.SetInsertPoint(success_block);
    return result;
}

template<typename F>
void perform_store(JIT::Context& context, llvm::Function* f, llvm::Value* value, F get_address)
{
    llvm::Value* did_succeed = context.builder.CreateCall(f, {
        get_address(context),
        value,
        u64_im(context.pc)
    });

    // Return early on failure
    llvm::Function* function = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* success_block = llvm::BasicBlock::Create(context.context);
    llvm::BasicBlock* failure_block = llvm::BasicBlock::Create(context.context);
    context.builder.CreateCondBr(did_succeed, success_block, failure_block);

    // Failure block - unable to JIT further (for now!) so return early
    context.builder.SetInsertPoint(failure_block);
    context.builder.CreateRet(u64_im(0));
    function->insert(function->end(), failure_block);

    // Success block - carry on
    context.builder.SetInsertPoint(success_block);
    function->insert(function->end(), success_block);
}

inline void create_non_terminating_return(JIT::Context& context, llvm::Value* pc, llvm::Value* condition = nullptr)
{
    if (condition == nullptr)
        condition = u1_im(true);

    llvm::Function* root = context.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* return_block = llvm::BasicBlock::Create(context.context);
    llvm::BasicBlock* failure_block = llvm::BasicBlock::Create(context.context);
    context.builder.CreateCondBr(condition, return_block, failure_block);

    context.builder.SetInsertPoint(return_block);
    context.builder.CreateRet(pc);
    root->insert(root->end(), return_block);

    context.builder.SetInsertPoint(failure_block);
    root->insert(root->end(), failure_block);
}

inline void call_handler_and_return(JIT::Context& context, llvm::Function* f)
{
    create_non_terminating_return(
        context,
        u64_im(context.pc + 4),
        context.builder.CreateCall(f, { u64_im(context.pc) })
    );
}
