#pragma once
#include "cpu.h"
#include "instruction.h"

#define OPCODES_A           0b0101111
#define OPCODES_A_FUNCT_3   0b010
#define LR_W                0b00010
#define SC_W                0b00011
#define AMOSWAP_W           0b00001
#define AMOADD_W            0b00000
#define AMOXOR_W            0b00100
#define AMOAND_W            0b01100
#define AMOOR_W             0b01000
#define AMOMIN_W            0b10000
#define AMOMAX_W            0b10100
#define AMOMINU_W           0b11000
#define AMOMAXU_W           0b11100

#define OPCODES_A_64        0b011
#define LR_D                0b00010
#define SC_D                0b00011
#define AMOSWAP_D           0b00001
#define AMOADD_D            0b00000
#define AMOXOR_D            0b00100
#define AMOAND_D            0b01100
#define AMOOR_D             0b01000
#define AMOMIN_D            0b10000
#define AMOMAX_D            0b10100
#define AMOMINU_D           0b11000
#define AMOMAXU_D           0b11100

bool opcodes_a(CPU& cpu, const Instruction instruction);

void lr_w       (CPU& cpu, const Instruction instruction);
void sc_w       (CPU& cpu, const Instruction instruction);
void amoswap_w  (CPU& cpu, const Instruction instruction);
void amoadd_w   (CPU& cpu, const Instruction instruction);
void amoxor_w   (CPU& cpu, const Instruction instruction);
void amoand_w   (CPU& cpu, const Instruction instruction);
void amoor_w    (CPU& cpu, const Instruction instruction);
void amomin_w   (CPU& cpu, const Instruction instruction);
void amomax_w   (CPU& cpu, const Instruction instruction);
void amominu_w  (CPU& cpu, const Instruction instruction);
void amomaxu_w  (CPU& cpu, const Instruction instruction);

void lr_d       (CPU& cpu, const Instruction instruction);
void sc_d       (CPU& cpu, const Instruction instruction);
void amoswap_d  (CPU& cpu, const Instruction instruction);
void amoadd_d   (CPU& cpu, const Instruction instruction);
void amoxor_d   (CPU& cpu, const Instruction instruction);
void amoand_d   (CPU& cpu, const Instruction instruction);
void amoor_d    (CPU& cpu, const Instruction instruction);
void amomin_d   (CPU& cpu, const Instruction instruction);
void amomax_d   (CPU& cpu, const Instruction instruction);
void amominu_d  (CPU& cpu, const Instruction instruction);
void amomaxu_d  (CPU& cpu, const Instruction instruction);
