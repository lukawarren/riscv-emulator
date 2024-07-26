#pragma once
#include "cpu.h"
#include "instruction.h"

#define OPCODES_F_1 0x27
#define OPCODES_F_2 0x43
#define OPCODES_F_3 0x47
#define OPCODES_F_4 0x4b
#define OPCODES_F_5 0x4f
#define OPCODES_F_6 0x53

bool opcodes_f(CPU& cpu, const Instruction& instruction);
