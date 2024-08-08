#pragma once
#include "cpu.h"
#include "instruction.h"
#include "jit/llvm.h"
#include "jit/jit.h"

namespace JIT
{
    void csrrw (const Instruction instruction, Context& context);
    void csrrc (const Instruction instruction, Context& context);
    void csrrs (const Instruction instruction, Context& context);
    void csrrwi(const Instruction instruction, Context& context);
    void csrrsi(const Instruction instruction, Context& context);
    void csrrci(const Instruction instruction, Context& context);
}