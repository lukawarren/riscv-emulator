#pragma once
#include "jit/jit.h"

namespace JIT
{
    void lr_w     (Context& context);
    void sc_w     (Context& context);
    void amoswap_w(Context& context);
    void amoadd_w (Context& context);
    void amoxor_w (Context& context);
    void amoand_w (Context& context);
    void amoor_w  (Context& context);
    void amomin_w (Context& context);
    void amomax_w (Context& context);
    void amominu_w(Context& context);
    void amomaxu_w(Context& context);

    void lr_d     (Context& context);
    void sc_d     (Context& context);
    void amoswap_d(Context& context);
    void amoadd_d (Context& context);
    void amoxor_d (Context& context);
    void amoand_d (Context& context);
    void amoor_d  (Context& context);
    void amomin_d (Context& context);
    void amomax_d (Context& context);
    void amominu_d(Context& context);
    void amomaxu_d(Context& context);
}