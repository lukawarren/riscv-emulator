#pragma once
#include "cpu.h"
#include "compressed_instruction.h"

#define OPCODES_C           0b11

#define C_LW                0b010
#define C_LD                0b011
#define C_LWSP              0b010
#define C_LDSP              0b011
#define C_SW                0b110
#define C_SD                0b111
#define C_SWSP              0b110
#define C_SDSP              0b111
#define C_J                 0b101
#define C_BEQZ              0b110
#define C_BNEZ              0b111
#define C_LI                0b010
#define C_ADDI              0b000
#define C_ADDIW             0b001
#define C_ADDI16SP          0b011
#define C_ADDI4SPN          0b000
#define C_SLLI              0b000
#define C_SRLI              0b000
#define C_SRAI              0b001
#define C_ANDI              0b010
#define C_NOP               0b000

bool opcodes_c  (CPU& cpu, const CompressedInstruction instruction);

void c_lw       (CPU& cpu, const CompressedInstruction instruction);
void c_ld       (CPU& cpu, const CompressedInstruction instruction);
void c_lwsp     (CPU& cpu, const CompressedInstruction instruction);
void c_ldsp     (CPU& cpu, const CompressedInstruction instruction);
void c_sw       (CPU& cpu, const CompressedInstruction instruction);
void c_sd       (CPU& cpu, const CompressedInstruction instruction);
void c_swsp     (CPU& cpu, const CompressedInstruction instruction);
void c_sdsp     (CPU& cpu, const CompressedInstruction instruction);
void c_j        (CPU& cpu, const CompressedInstruction instruction);
void c_jr       (CPU& cpu, const CompressedInstruction instruction);
void c_jalr     (CPU& cpu, const CompressedInstruction instruction);
void c_beqz     (CPU& cpu, const CompressedInstruction instruction);
void c_bnez     (CPU& cpu, const CompressedInstruction instruction);
void c_li       (CPU& cpu, const CompressedInstruction instruction);
void c_lui      (CPU& cpu, const CompressedInstruction instruction);
void c_addi     (CPU& cpu, const CompressedInstruction instruction);
void c_addiw    (CPU& cpu, const CompressedInstruction instruction);
void c_addi16sp (CPU& cpu, const CompressedInstruction instruction);
void c_addi4spn (CPU& cpu, const CompressedInstruction instruction);
void c_slli     (CPU& cpu, const CompressedInstruction instruction);
void c_srli     (CPU& cpu, const CompressedInstruction instruction);
void c_srai     (CPU& cpu, const CompressedInstruction instruction);
void c_andi     (CPU& cpu, const CompressedInstruction instruction);
void c_mv       (CPU& cpu, const CompressedInstruction instruction);
void c_add      (CPU& cpu, const CompressedInstruction instruction);
void c_addw     (CPU& cpu, const CompressedInstruction instruction);
void c_and      (CPU& cpu, const CompressedInstruction instruction);
void c_or       (CPU& cpu, const CompressedInstruction instruction);
void c_xor      (CPU& cpu, const CompressedInstruction instruction);
void c_sub      (CPU& cpu, const CompressedInstruction instruction);
void c_subw     (CPU& cpu, const CompressedInstruction instruction);
void c_ebreak   (CPU& cpu, const CompressedInstruction instruction);

