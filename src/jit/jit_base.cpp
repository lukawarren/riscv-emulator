#include "jit/jit_base.h"
#include "jit/jit_common.h"

using namespace JIT;

// Helpers
static void emit_branch(Context& context, llvm::Value* condition);
static llvm::Value* sign_extend_32_as_64(Context& context, llvm::Value* value);
static llvm::Value* sign_extend(Context& context, llvm::Value* value);
static llvm::Value* zero_extend(Context& context, llvm::Value* value);
static llvm::Value* get_load_address(Context& context);
static llvm::Value* get_store_address(Context& context);
static llvm::Value* perform_load(Context& context, llvm::Function* f);
static void perform_store(Context& context, llvm::Function* f, llvm::Value* value);

void JIT::add(Context& context)
{
    set_rd(context.builder.CreateAdd(rs1, rs2));
}

void JIT::sub(Context& context)
{
    set_rd(context.builder.CreateSub(rs1, rs2));
}

void JIT::_xor(Context& context)
{
    set_rd(context.builder.CreateXor(rs1, rs2));
}

void JIT::_or(Context& context)
{
    set_rd(context.builder.CreateOr(rs1, rs2));
}

void JIT::_and(Context& context)
{
    set_rd(context.builder.CreateAnd(rs1, rs2));
}

void JIT::sll(Context& context)
{
    llvm::Value* _rs2 = load_register(context, context.current_instruction.get_rs2_6_bits());
    set_rd(context.builder.CreateShl(rs1, _rs2));
}

void JIT::srl(Context& context)
{
    llvm::Value* _rs2 = load_register(context, context.current_instruction.get_rs2_6_bits());
    set_rd(context.builder.CreateLShr(rs1, _rs2));
}

void JIT::sra(Context& context)
{
    llvm::Value* _rs2 = load_register(context, context.current_instruction.get_rs2_6_bits());
    set_rd(context.builder.CreateAShr(rs1, _rs2));
}

void JIT::slt(Context& context)
{
    // rd = ((i64) rs1 < (i64)rs2) ? 1 : 0
    llvm::Value* comparison = context.builder.CreateICmpSLT(rs1, rs2);
    set_rd(context.builder.CreateSelect(
        comparison,
        u64_im(1),
        u64_im(0)
    ));
}

void JIT::sltu(Context& context)
{
    // rd = (rs1 < rs2) ? 1 : 0
    llvm::Value* comparison = context.builder.CreateICmpULT(rs1, rs2);
    set_rd(context.builder.CreateSelect(
        comparison,
        u64_im(1),
        u64_im(0)
    ));
}

void JIT::addi(Context& context)
{
    llvm::Value* imm = u64_im(context.current_instruction.get_imm(Instruction::Type::I));
    set_rd(context.builder.CreateAdd(
        rs1,
        imm
    ));
}

void JIT::xori(Context& context)
{
    llvm::Value* imm = u64_im(context.current_instruction.get_imm(Instruction::Type::I));
    set_rd(context.builder.CreateXor(
        rs1,
        imm
    ));
}

void JIT::ori(Context& context)
{
    llvm::Value* imm = u64_im(context.current_instruction.get_imm(Instruction::Type::I));
    set_rd(context.builder.CreateOr(
        rs1,
        imm
    ));
}

void JIT::andi(Context& context)
{
    llvm::Value* imm = u64_im(context.current_instruction.get_imm(Instruction::Type::I));
    set_rd(context.builder.CreateAnd(
        rs1,
        imm
    ));
}

void JIT::slli(Context& context)
{
    llvm::Value* imm = u64_im(context.current_instruction.get_shamt());
    set_rd(context.builder.CreateShl(
        rs1,
        imm
    ));
}

void JIT::srli(Context& context)
{
    llvm::Value* imm = u64_im(context.current_instruction.get_shamt());
    set_rd(context.builder.CreateLShr(
        rs1,
        imm
    ));
}

void JIT::srai(Context& context)
{
    llvm::Value* imm = u64_im(context.current_instruction.get_shamt());
    set_rd(context.builder.CreateAShr(
        rs1,
        imm
    ));
}

void JIT::slti(Context& context)
{
    llvm::Value* imm = u64_im(context.current_instruction.get_imm(Instruction::Type::I));
    llvm::Value* comparison = context.builder.CreateICmpSLT(rs1, imm);
    set_rd(context.builder.CreateSelect(
        comparison,
        u64_im(1),
        u64_im(0)
    ));
}

void JIT::sltiu(Context& context)
{
    llvm::Value* imm = u64_im(context.current_instruction.get_imm(Instruction::Type::I));
    llvm::Value* comparison = context.builder.CreateICmpULT(rs1, imm);
    set_rd(context.builder.CreateSelect(
        comparison,
        u64_im(1),
        u64_im(0)
    ));
}

void JIT::lb(Context& context)
{
    set_rd(sign_extend(context, perform_load(context, context.on_lb)));
}

void JIT::lh(Context& context)
{
    set_rd(sign_extend(context, perform_load(context, context.on_lh)));
}

void JIT::lw(Context& context)
{
    set_rd(sign_extend(context, perform_load(context, context.on_lw)));
}

void JIT::lbu(Context& context)
{
    set_rd(zero_extend(context, perform_load(context, context.on_lb)));
}

void JIT::lhu(Context& context)
{
    set_rd(zero_extend(context, perform_load(context, context.on_lh)));
}

void JIT::sb(Context& context)
{
    perform_store(context, context.on_sb, u64_to_8(rs2));
}

void JIT::sh(Context& context)
{
    perform_store(context, context.on_sh, u64_to_16(rs2));
}

void JIT::sw(Context& context)
{
    perform_store(context, context.on_sw, u64_to_32(rs2));
}

void JIT::beq(Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpEQ(rs1, rs2);
    emit_branch(context, condition);
}

void JIT::bne(Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpNE(rs1, rs2);
    emit_branch(context, condition);
}

void JIT::blt(Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpSLT(rs1, rs2);
    emit_branch(context, condition);
}

void JIT::bge(Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpSGE(rs1, rs2);
    emit_branch(context, condition);
}

void JIT::bltu(Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpULT(rs1, rs2);
    emit_branch(context, condition);
}

void JIT::bgeu(Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpUGE(rs1, rs2);
    emit_branch(context, condition);
}

void JIT::jal(Context& context)
{
    // Instead of actually running a jump, we can just simulate the effects
    const i64 offset = context.current_instruction.get_imm(Instruction::Type::J);
    store_register(
        context,
        context.current_instruction.get_rd(),
        u64_im(context.pc + 4)
    );
    context.pc += offset - 4;
}

void JIT::jalr(Context& context)
{
    // Offset always has LSB set to zero
    const i64 imm = context.current_instruction.get_imm(Instruction::Type::I);
    llvm::Value* offset = context.builder.CreateAnd(
        context.builder.CreateAdd(u64_im(imm), rs1),
        0xfffffffffffffffe
    );

    store_register(
        context,
        context.current_instruction.get_rd(),
        u64_im(context.pc + 4)
    );

    // We don't know the final PC at compile time so must return
    context.builder.CreateRet(offset);
    context.emitted_jalr = true;
}

void JIT::lui(Context& context)
{
    set_rd(u64_im(context.current_instruction.get_imm(Instruction::Type::U)));
}

void JIT::auipc(Context& context)
{
    set_rd(u64_im(
        context.pc + context.current_instruction.get_imm(Instruction::Type::U)
    ));
}

void JIT::ecall(Context& context)
{
    // TODO: don't stop translation
    context.builder.CreateCall(context.on_ecall, { u64_im(context.pc) });
    stop_translation();
}

void JIT::ebreak(Context& context)
{
    // TODO: don't stop translation
    context.builder.CreateCall(context.on_ebreak, { u64_im(context.pc) });
    stop_translation();
}

void JIT::uret(Context& context)
{
    // TODO: don't stop translation
    context.builder.CreateCall(context.on_uret, { u64_im(context.pc) });
    stop_translation();
}

void JIT::sret(Context& context)
{
    // TODO: don't stop translation
    context.builder.CreateCall(context.on_sret, { u64_im(context.pc) });
    stop_translation();
}

void JIT::mret(Context& context)
{
    // TODO: don't stop translation
    context.builder.CreateCall(context.on_mret, { u64_im(context.pc) });
    stop_translation();
}

void JIT::wfi(Context& context)
{
    // TODO: don't stop translation
    context.builder.CreateCall(context.on_wfi, { u64_im(context.pc) });
    stop_translation();
}

void JIT::sfence_vma(Context& context)
{
    // TLB invalidated so the JIT is too
    context.builder.CreateCall(context.on_sfence_vma, { u64_im(context.pc) });
    stop_translation();
}

void JIT::lwu(Context& context)
{
    set_rd(zero_extend(context, perform_load(context, context.on_lw)));
}

void JIT::ld(Context& context)
{
    set_rd(perform_load(context, context.on_ld));
}

void JIT::sd(Context& context)
{
    perform_store(context, context.on_sd, rs2);
}

void JIT::addiw(Context& context)
{
    llvm::Value* imm  = u64_im(context.current_instruction.get_imm(Instruction::Type::I));
    llvm::Value* result = context.builder.CreateAdd(imm, rs1);
    set_rd(sign_extend_32_as_64(context, result));
}

void JIT::slliw(Context& context)
{
    u32 shamt = context.current_instruction.get_wide_shift_amount();
    llvm::Value* result = context.builder.CreateShl(rs1, shamt);
    set_rd(sign_extend_32_as_64(context, result));
}

void JIT::srliw(Context& context)
{
    u32 shamt = context.current_instruction.get_wide_shift_amount();
    llvm::Value* _rs1 = u64_to_32(rs1);
    llvm::Value* result = context.builder.CreateLShr(_rs1, shamt);
    set_rd(sign_extend_32_as_64(context, result));
}

void JIT::sraiw(Context& context)
{
    u32 shamt = context.current_instruction.get_wide_shift_amount();
    llvm::Value* _rs1 = u64_to_32(rs1);
    llvm::Value* result = context.builder.CreateAShr(_rs1, shamt);
    set_rd(sign_extend_32_as_64(context, result));
}

void JIT::addw(Context& context)
{
    llvm::Value* result = context.builder.CreateAdd(rs1, rs2);
    set_rd(sign_extend_32_as_64(context, result));
}

void JIT::subw(Context& context)
{
    llvm::Value* result = context.builder.CreateSub(rs1, rs2);
    set_rd(sign_extend_32_as_64(context, result));
}

void JIT::sllw(Context& context)
{
    llvm::Value* _rs1 = u64_to_32(rs1);
    llvm::Value* _rs2 = u64_to_32(context.builder.CreateAnd(rs2, u64_im(0b11111)));
    llvm::Value* result = context.builder.CreateShl(_rs1, _rs2);
    set_rd(sign_extend_32_as_64(context, result));
}

void JIT::srlw(Context& context)
{
    llvm::Value* _rs1 = u64_to_32(rs1);
    llvm::Value* _rs2 = u64_to_32(context.builder.CreateAnd(rs2, u64_im(0b11111)));
    llvm::Value* result = context.builder.CreateLShr(_rs1, _rs2);
    set_rd(sign_extend_32_as_64(context, result));
}

void JIT::sraw(Context& context)
{
    llvm::Value* _rs1 = u64_to_32(rs1);
    llvm::Value* _rs2 = u64_to_32(context.builder.CreateAnd(rs2, u64_im(0b11111)));
    llvm::Value* result = context.builder.CreateAShr(_rs1, _rs2);
    set_rd(sign_extend_32_as_64(context, result));
}

// -- Helpers --

static void emit_branch(Context& context, llvm::Value* condition)
{
    // Check branch alignment
    const u64 target = context.current_instruction.get_imm(Instruction::Type::B);
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

static llvm::Value* sign_extend_32_as_64(Context& context, llvm::Value* value)
{
    return context.builder.CreateSExt(
        u64_to_32(value),
        context.builder.getInt64Ty()
    );
}

static llvm::Value* sign_extend(Context& context, llvm::Value* value)
{
    return context.builder.CreateSExt(
        value,
        context.builder.getInt64Ty()
    );
}

static llvm::Value* zero_extend(Context& context, llvm::Value* value)
{
    return context.builder.CreateZExt(
        value,
        context.builder.getInt64Ty()
    );
}

static llvm::Value* get_load_address(Context& context)
{
    return context.builder.CreateAdd(
        u64_im(context.current_instruction.get_imm(Instruction::Type::I)),
        rs1
    );
}

static llvm::Value* get_store_address(Context& context)
{
    return context.builder.CreateAdd(
        u64_im(context.current_instruction.get_imm(Instruction::Type::S)),
        rs1
    );
}

static llvm::Value* perform_load(Context& context, llvm::Function* f)
{
    const auto bool_type = llvm::Type::getInt1Ty(context.context);
    llvm::Value* did_succeed_ptr = context.builder.CreateAlloca(bool_type);
    llvm::Value* result = context.builder.CreateCall(f, {
        get_load_address(context),
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
    context.builder.CreateRet(u64_im(context.pc + 4));
    function->insert(function->end(), failure_block);

    // Success block - carry on
    function->insert(function->end(), success_block);
    context.builder.SetInsertPoint(success_block);
    return result;
}

static void perform_store(Context& context, llvm::Function* f, llvm::Value* value)
{
    llvm::Value* did_succeed = context.builder.CreateCall(f, {
        get_store_address(context),
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
    context.builder.CreateRet(u64_im(context.pc + 4));
    function->insert(function->end(), failure_block);

    // Success block - carry on
    function->insert(function->end(), success_block);
    context.builder.SetInsertPoint(success_block);
}
