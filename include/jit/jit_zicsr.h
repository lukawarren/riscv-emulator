#pragma once
#include "jit/jit.h"

namespace JIT
{
    void csrrw (Context& context);
    void csrrc (Context& context);
    void csrrs (Context& context);
    void csrrwi(Context& context);
    void csrrsi(Context& context);
    void csrrci(Context& context);
}