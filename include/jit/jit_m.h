#pragma once
#include "jit/jit.h"

namespace JIT
{
    void mul    (Context& context);
    void mulh   (Context& context);
    void mulhsu (Context& context);
    void mulhu  (Context& context);
    void div    (Context& context);
    void divu   (Context& context);
    void rem    (Context& context);
    void remu   (Context& context);

    void mulw   (Context& context);
    void divw   (Context& context);
    void divuw  (Context& context);
    void remw   (Context& context);
    void remuw  (Context& context);
}
