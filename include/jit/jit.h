#pragma once
#include "common.h"
#include "jit/llvm.h"
#include "instruction.h"
#include "compressed_instruction.h"
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
        ) : builder(builder), context(context), pc(pc),
            current_instruction(0), current_compressed_instruction(0) {}

        // Variables
        llvm::Value* registers;
        u64 pc;
        Instruction current_instruction;
        CompressedInstruction current_compressed_instruction;

        // Base interface functions
        llvm::Function* on_ecall;
        llvm::Function* on_ebreak;
        llvm::Function* on_c_ebreak;
        llvm::Function* on_uret;
        llvm::Function* on_sret;
        llvm::Function* on_mret;
        llvm::Function* on_wfi;
        llvm::Function* on_sfence_vma;
        llvm::Function* on_lb;
        llvm::Function* on_lh;
        llvm::Function* on_lw;
        llvm::Function* on_ld;
        llvm::Function* on_sb;
        llvm::Function* on_sh;
        llvm::Function* on_sw;
        llvm::Function* on_sd;
        llvm::Function* set_fcsr_dz;

        // Fallbacks for "tricky" extensions that infrequently pop-up;
        // negligable for performance in most cases
        llvm::Function* on_csr;
        llvm::Function* on_atomic;
        llvm::Function* on_floating;
        llvm::Function* on_floating_compressed;

        bool abort_translation = false;
    };

    template <typename T>
    struct Optional
    {
        bool has_value;
        T value;

        Optional() : has_value(false) {}
        Optional(T value) : has_value(true), value(value) {}
    };

    struct Frame
    {
        llvm::ExecutionEngine* engine;
        u64 starting_pc;
        u64 ending_pc;

        Frame(llvm::ExecutionEngine* engine, u64 starting_pc, u64 ending_pc) :
            engine(engine), starting_pc(starting_pc), ending_pc(ending_pc) {}
    };

    void init();
    void run_next_frame(
        CPU& cpu
    );
    std::optional<Frame> compile_next_frame(
        CPU& cpu
    );
    void execute_frame(
        CPU& cpu,
        Frame& frame,
        u64 pc
    );
    void cache_frame(
        Frame& frame
    );
    bool check_for_exceptions(
        CPU& cpu
    );
    std::optional<Frame> get_cached_frame(
        u64 pc
    );
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
        Context& context
    );
    bool emit_compressed_instruction(
        CPU& cpu,
        Context& context
    );

    llvm::Value* get_registers(CPU& cpu, llvm::IRBuilder<>& builder);
    llvm::Value* load_register(Context& context, u32 index);
    void store_register(Context& context, u32 index, llvm::Value* value);
;}
