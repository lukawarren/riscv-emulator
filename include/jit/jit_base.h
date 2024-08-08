#pragma once
#include "cpu.h"
#include "instruction.h"
#include "jit/llvm.h"
#include "jit/jit.h"

#define UNIMPLEMENTED() throw std::runtime_error("unimplemented: " + std::to_string(__LINE__));

namespace JIT
{
    void add        (const Instruction instruction, Context& context);
    void sub        (const Instruction instruction, Context& context);
    void _xor       (const Instruction instruction, Context& context);
    void _or        (const Instruction instruction, Context& context);
    void _and       (const Instruction instruction, Context& context);
    void sll        (const Instruction instruction, Context& context);
    void srl        (const Instruction instruction, Context& context);
    void sra        (const Instruction instruction, Context& context);
    void slt        (const Instruction instruction, Context& context);
    void sltu       (const Instruction instruction, Context& context);

    void addi       (const Instruction instruction, Context& context);
    void xori       (const Instruction instruction, Context& context);
    void ori        (const Instruction instruction, Context& context);
    void andi       (const Instruction instruction, Context& context);
    void slli       (const Instruction instruction, Context& context);
    void srli       (const Instruction instruction, Context& context);
    void srai       (const Instruction instruction, Context& context);
    void slti       (const Instruction instruction, Context& context);
    void sltiu      (const Instruction instruction, Context& context);

    inline void lb         (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void lh         (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void lw         (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void lbu        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void lhu        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

    inline void sb         (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sh         (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sw         (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

    inline void beq        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void bne        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void blt        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void bge        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void bltu       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void bgeu       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

    void jal        (const Instruction instruction, Context& context);
    inline void jalr       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

    inline void lui        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void auipc      (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

    inline void ecall      (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void ebreak     (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void uret       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sret       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void mret       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void wfi        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sfence_vma (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

    inline void lwu        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void ld         (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sd         (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

    inline void addiw      (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void slliw      (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void srliw      (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sraiw      (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

    inline void addw       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void subw       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sllw       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void srlw       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sraw       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
}