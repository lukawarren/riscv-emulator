#pragma once
#include "jit/jit_base.h"

namespace JIT
{
    void c_lw       (Context& context);
    void c_ld       (Context& context);
    void c_lwsp     (Context& context);
    void c_ldsp     (Context& context);
    void c_sw       (Context& context);
    void c_sd       (Context& context);
    void c_swsp     (Context& context);
    void c_sdsp     (Context& context);
    void c_j        (Context& context);
    void c_jr       (Context& context);
    void c_jalr     (Context& context);
    void c_beqz     (Context& context);
    void c_bnez     (Context& context);
    void c_li       (Context& context);
    void c_lui      (Context& context);
    void c_addi     (Context& context);
    void c_addiw    (Context& context);
    void c_addi16sp (Context& context);
    void c_addi4spn (Context& context);
    void c_slli     (Context& context);
    void c_srli     (Context& context);
    void c_srai     (Context& context);
    void c_andi     (Context& context);
    void c_mv       (Context& context);
    void c_add      (Context& context);
    void c_addw     (Context& context);
    void c_and      (Context& context);
    void c_or       (Context& context);
    void c_xor      (Context& context);
    void c_sub      (Context& context);
    void c_subw     (Context& context);
    void c_ebreak   (Context& context);
}