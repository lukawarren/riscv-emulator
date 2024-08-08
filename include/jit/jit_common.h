#pragma once
#define rs1 load_register(context, instruction.get_rs1())
#define rs2 load_register(context, instruction.get_rs2())
#define set_rd(r) store_register(context, instruction.get_rd(), r);
#define u64_im(x) llvm::ConstantInt::get(context.builder.getInt64Ty(), x)
