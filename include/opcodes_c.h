#pragma once
#include "cpu.h"
#include "compressed_instruction.h"

#define OPCODES_C           0b11

#define C_LW                0b010
#define C_SW                0b110
#define C_ADDI              0b000
#define C_ADDI16SP          0b011
#define C_ADDI4SPN          0b000
#define C_SLLI              0b000
#define C_NOP               0b000

bool opcodes_c  (CPU& cpu, const CompressedInstruction& instruction);

void c_lw       (CPU& cpu, const CompressedInstruction& instruction);
void c_sw       (CPU& cpu, const CompressedInstruction& instruction);

void c_addi     (CPU& cpu, const CompressedInstruction& instruction);
void c_addi16sp (CPU& cpu, const CompressedInstruction& instruction);
void c_addi4spn (CPU& cpu, const CompressedInstruction& instruction);

void c_slli     (CPU& cpu, const CompressedInstruction& instruction);
