#include "jit/jit_base.h"

using namespace JIT;

#define rs1 load_register(context, instruction.get_rs1())
#define rs2 load_register(context, instruction.get_rs2())
#define set_rd(r) store_register(context, instruction.get_rd(), r);
#define u64_im(x) llvm::ConstantInt::get(context.builder.getInt64Ty(), x)

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
