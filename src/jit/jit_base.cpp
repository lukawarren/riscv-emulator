#include "jit/jit_base.h"

using namespace JIT;

void JIT::add(const Instruction instruction, Context& context)
{
    llvm::Value* rs1 = load_register(context, instruction.get_rs1());
    llvm::Value* rs2 = load_register(context, instruction.get_rs2());
    llvm::Value* result = context.builder.CreateAdd(rs1, rs2);
    store_register(context, instruction.get_rd(), result);
}

void JIT::jal(const Instruction instruction, Context& context)
{
    // Instead of actually running a jump, we can just simulate the effects
    const i64 offset = instruction.get_imm(Instruction::Type::J);
    store_register(
        context,
        instruction.get_rd(),
        llvm::ConstantInt::get(context.builder.getInt64Ty(), context.pc + 4)
    );
    context.pc += offset - 4;
}
