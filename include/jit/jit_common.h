#pragma once

#define rs1 load_register(context, instruction.get_rs1())
#define rs2 load_register(context, instruction.get_rs2())
#define set_rd(r) store_register(context, instruction.get_rd(), r)
#define u32_im(x) llvm::ConstantInt::get(context.builder.getInt32Ty(), x)
#define u64_im(x) llvm::ConstantInt::get(context.builder.getInt64Ty(), x)
#define stop_translation() context.return_pc = context.pc + 4
#define has_value(x) context.builder.CreateExtractValue(x, 0);
#define get_value(x) context.builder.CreateExtractValue(x, 1);