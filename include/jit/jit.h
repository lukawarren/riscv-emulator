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

        Context(
            llvm::IRBuilder<>& builder,
            llvm::LLVMContext& context,
            u64 pc
        ) : builder(builder), context(context), pc(pc), current_instruction(0) {}

        // Variables
        llvm::Value* registers;
        u64 pc;

        // Interface functions
        llvm::Function* on_csr;
        llvm::Function* on_ecall;
        llvm::Function* on_mret;
        llvm::Function* on_branch_fail;

        // Early return
        std::optional<u64> return_pc = std::nullopt;

        // For falling back to the interpreter
        Instruction current_instruction;
    };

    void init();
    void run_next_frame(CPU& cpu);
    void register_interface_functions(
        llvm::Module* module,
        llvm::LLVMContext& context,
        Context& jit_context
    );
    void link_interface_functions(
        llvm::ExecutionEngine* engine,
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