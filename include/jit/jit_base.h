#pragma once
#include "cpu.h"
#include "instruction.h"
#include "jit/llvm.h"
#include "jit/jit.h"

#define UNIMPLEMENTED() throw std::runtime_error("unimplemented: " + std::to_string(__LINE__));

namespace JIT
{
    void add        (const Instruction instruction, Context& context);
    inline void sub        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void _xor       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void _or        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void _and       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sll        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void srl        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sra        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void slt        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sltu       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

    inline void addi       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void xori       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void ori        (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void andi       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void slli       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void srli       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void srai       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void slti       (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }
    inline void sltiu      (const Instruction instruction, Context& context) { UNIMPLEMENTED(); }

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