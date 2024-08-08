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
        llvm::LLVMContext& context;

        // Variables
        llvm::Value* registers;
        u64 pc;

        // Interface functions
        llvm::Function* write_to_csr;
    };

    void init();
    void create_frame(CPU& cpu);
    void register_interface_functions(
        llvm::Module* module,
        llvm::LLVMContext& context,
        Context& jit_context
    );
    bool emit_instruction(
        CPU& cpu,
        const Instruction instruction,
        Context& context
    );

    llvm::Value* get_registers(CPU& cpu, llvm::IRBuilder<>& builder);
    llvm::Value* load_register(Context& context, u32 index);
    void store_register(Context& context, u32 index, llvm::Value* value);
}