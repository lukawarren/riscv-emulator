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
#define stop_translation() context.return_pc = context.pc + 4
