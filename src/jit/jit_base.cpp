#include "jit/jit_base.h"
#include "jit/jit_common.h"

using namespace JIT;

// Helpers
static void emit_branch(const Instruction instruction, Context& context, llvm::Value* condition);
static llvm::Value* sign_extend_32(Context& context, llvm::Value* value);

void JIT::add(const Instruction instruction, Context& context)
{
    set_rd(context.builder.CreateAdd(rs1, rs2));
}

void JIT::sub(const Instruction instruction, Context& context)
{
    set_rd(context.builder.CreateSub(rs1, rs2));
}

void JIT::_xor(const Instruction instruction, Context& context)
{
    set_rd(context.builder.CreateXor(rs1, rs2));
}

void JIT::_or(const Instruction instruction, Context& context)
{
    set_rd(context.builder.CreateOr(rs1, rs2));
}

void JIT::_and(const Instruction instruction, Context& context)
{
    set_rd(context.builder.CreateAnd(rs1, rs2));
}

void JIT::sll(const Instruction instruction, Context& context)
{
    llvm::Value* _rs2 = load_register(context, instruction.get_rs2_6_bits());
    set_rd(context.builder.CreateShl(rs1, _rs2));
}

void JIT::srl(const Instruction instruction, Context& context)
{
    llvm::Value* _rs2 = load_register(context, instruction.get_rs2_6_bits());
    set_rd(context.builder.CreateLShr(rs1, _rs2));
}

void JIT::sra(const Instruction instruction, Context& context)
{
    llvm::Value* _rs2 = load_register(context, instruction.get_rs2_6_bits());
    set_rd(context.builder.CreateAShr(rs1, _rs2));
}

void JIT::slt(const Instruction instruction, Context& context)
{
    // rd = ((i64) rs1 < (i64)rs2) ? 1 : 0
    llvm::Value* comparison = context.builder.CreateICmpSLT(rs1, rs2);
    set_rd(context.builder.CreateSelect(
        comparison,
        u64_im(1),
        u64_im(0)
    ));
}

void JIT::sltu(const Instruction instruction, Context& context)
{
    // rd = (rs1 < rs2) ? 1 : 0
    llvm::Value* comparison = context.builder.CreateICmpULT(rs1, rs2);
    set_rd(context.builder.CreateSelect(
        comparison,
        u64_im(1),
        u64_im(0)
    ));
}

void JIT::addi(const Instruction instruction, Context& context)
{
    llvm::Value* imm = u64_im(instruction.get_imm(Instruction::Type::I));
    set_rd(context.builder.CreateAdd(
        rs1,
        imm
    ));
}

void JIT::xori(const Instruction instruction, Context& context)
{
    llvm::Value* imm = u64_im(instruction.get_imm(Instruction::Type::I));
    set_rd(context.builder.CreateXor(
        rs1,
        imm
    ));
}

void JIT::ori(const Instruction instruction, Context& context)
{
    llvm::Value* imm = u64_im(instruction.get_imm(Instruction::Type::I));
    set_rd(context.builder.CreateOr(
        rs1,
        imm
    ));
}

void JIT::andi(const Instruction instruction, Context& context)
{
    llvm::Value* imm = u64_im(instruction.get_imm(Instruction::Type::I));
    set_rd(context.builder.CreateAnd(
        rs1,
        imm
    ));
}

void JIT::slli(const Instruction instruction, Context& context)
{
    llvm::Value* imm = u64_im(instruction.get_shamt());
    set_rd(context.builder.CreateShl(
        rs1,
        imm
    ));
}

void JIT::srli(const Instruction instruction, Context& context)
{
    llvm::Value* imm = u64_im(instruction.get_shamt());
    set_rd(context.builder.CreateLShr(
        rs1,
        imm
    ));
}

void JIT::srai(const Instruction instruction, Context& context)
{
    llvm::Value* imm = u64_im(instruction.get_shamt());
    set_rd(context.builder.CreateAShr(
        rs1,
        imm
    ));
}

void JIT::slti(const Instruction instruction, Context& context)
{
    llvm::Value* imm = u64_im(instruction.get_imm(Instruction::Type::I));
    llvm::Value* comparison = context.builder.CreateICmpSLT(rs1, imm);
    set_rd(context.builder.CreateSelect(
        comparison,
        u64_im(1),
        u64_im(0)
    ));
}

void JIT::sltiu(const Instruction instruction, Context& context)
{
    llvm::Value* imm = u64_im(instruction.get_imm(Instruction::Type::I));
    llvm::Value* comparison = context.builder.CreateICmpULT(rs1, imm);
    set_rd(context.builder.CreateSelect(
        comparison,
        u64_im(1),
        u64_im(0)
    ));
}

void JIT::beq(const Instruction instruction, Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpEQ(rs1, rs2);
    emit_branch(instruction, context, condition);
}

void JIT::bne(const Instruction instruction, Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpNE(rs1, rs2);
    emit_branch(instruction, context, condition);
}

void JIT::blt(const Instruction instruction, Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpSLT(rs1, rs2);
    emit_branch(instruction, context, condition);
}

void JIT::bge(const Instruction instruction, Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpSGE(rs1, rs2);
    emit_branch(instruction, context, condition);
}

void JIT::bltu(const Instruction instruction, Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpULT(rs1, rs2);
    emit_branch(instruction, context, condition);
}

void JIT::bgeu(const Instruction instruction, Context& context)
{
    llvm::Value* condition = context.builder.CreateICmpUGE(rs1, rs2);
    emit_branch(instruction, context, condition);
}

void JIT::jal(const Instruction instruction, Context& context)
{
    // Instead of actually running a jump, we can just simulate the effects
    const i64 offset = instruction.get_imm(Instruction::Type::J);
    store_register(
        context,
        instruction.get_rd(),
        u64_im(context.pc + 4)
    );
    context.pc += offset - 4;
}

void JIT::lui(const Instruction instruction, Context& context)
{
    set_rd(u64_im(instruction.get_imm(Instruction::Type::U)));
}

void JIT::auipc(const Instruction instruction, Context& context)
{
    set_rd(u64_im(
        context.pc + instruction.get_imm(Instruction::Type::U)
    ));
}

void JIT::ecall(const Instruction instruction, Context& context)
{
    context.builder.CreateCall(context.on_ecall, { u64_im(context.pc) });
}

void JIT::mret(const Instruction instruction, Context& context)
{
    context.builder.CreateCall(context.on_mret, { u64_im(context.pc) });
}

void JIT::addiw(const Instruction instruction, Context& context)
{
    llvm::Value* imm  = u64_im(instruction.get_imm(Instruction::Type::I));
    llvm::Value* result = context.builder.CreateAdd(imm, rs1);
    llvm::Value* masked = context.builder.CreateAnd(result, 0xffffffff);
    set_rd(sign_extend_32(context, masked));
}

void JIT::slliw(const Instruction instruction, Context& context)
{
    llvm::Value* result = context.builder.CreateShl(rs1, instruction.get_wide_shift_amount());
    set_rd(sign_extend_32(context, result));
}

void JIT::srliw(const Instruction instruction, Context& context)
{
    llvm::Value* _rs1 = context.builder.CreateTrunc(rs1, context.builder.getInt32Ty());
    llvm::Value* result = context.builder.CreateLShr(_rs1, instruction.get_wide_shift_amount());
    set_rd(sign_extend_32(context, result));
}

void JIT::sraiw(const Instruction instruction, Context& context)
{
    llvm::Value* _rs1 = context.builder.CreateTrunc(rs1, context.builder.getInt32Ty());
    llvm::Value* result = context.builder.CreateAShr(_rs1, instruction.get_wide_shift_amount());
    set_rd(sign_extend_32(context, result));
}

// -- Helpers --

static void emit_branch(const Instruction instruction, Context& context, llvm::Value* condition)
{
    // Check branch alignment
    const u64 target = instruction.get_imm(Instruction::Type::B);
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

static llvm::Value* sign_extend_32(Context& context, llvm::Value* value)
{
    return context.builder.CreateSExt(
        context.builder.CreateTrunc(value, context.builder.getInt32Ty()),
        context.builder.getInt64Ty()
    );
}
