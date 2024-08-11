#pragma once
#include "jit/jit.h"

#define UNIMPLEMENTED() throw std::runtime_error("unimplemented: " + std::to_string(__LINE__));

namespace JIT
{
    void add        (Context& context);
    void sub        (Context& context);
    void _xor       (Context& context);
    void _or        (Context& context);
    void _and       (Context& context);
    void sll        (Context& context);
    void srl        (Context& context);
    void sra        (Context& context);
    void slt        (Context& context);
    void sltu       (Context& context);

    void addi       (Context& context);
    void xori       (Context& context);
    void ori        (Context& context);
    void andi       (Context& context);
    void slli       (Context& context);
    void srli       (Context& context);
    void srai       (Context& context);
    void slti       (Context& context);
    void sltiu      (Context& context);

    void lb         (Context& context);
    void lh         (Context& context);
    void lw         (Context& context);
    void lbu        (Context& context);
    void lhu        (Context& context);

    void sb         (Context& context);
    void sh         (Context& context);
    void sw         (Context& context);

    void beq        (Context& context);
    void bne        (Context& context);
    void blt        (Context& context);
    void bge        (Context& context);
    void bltu       (Context& context);
    void bgeu       (Context& context);

    void jal        (Context& context);
    void jalr       (Context& context);

    void lui        (Context& context);
    void auipc      (Context& context);

    void ecall      (Context& context);
    void ebreak     (Context& context);
    void uret       (Context& context);
    void sret       (Context& context);
    void mret       (Context& context);
    void wfi        (Context& context);
    void sfence_vma (Context& context);

    void lwu        (Context& context);
    void ld         (Context& context);
    void sd         (Context& context);

    void addiw      (Context& context);
    void slliw      (Context& context);
    void srliw      (Context& context);
    void sraiw      (Context& context);

    void addw       (Context& context);
    void subw       (Context& context);
    void sllw       (Context& context);
    void srlw       (Context& context);
    void sraw       (Context& context);
}