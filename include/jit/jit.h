#pragma once
#include "common.h"
#include "jit/llvm.h"
#include "instruction.h"
#include "cpu.h"

namespace JIT
{
    struct Context
    {
        llvm::IRBuilder<>& builder;
        llvm::Value* registers;
        u64 pc;
    };

    void init();
    void create_frame(CPU& cpu);
    bool emit_instruction(CPU& cpu, const Instruction instruction, Context& context);

    llvm::Value* get_registers(CPU& cpu, llvm::IRBuilder<>& builder);
    llvm::Value* load_register(Context& context, u32 index);
    void store_register(Context& context, u32 index, llvm::Value* value);
}