#pragma once
#include "cpu.h"
#include "compressed_instruction.h"

#define OPCODES_C           0b11

bool opcodes_c(CPU& cpu, const CompressedInstruction& instruction);

