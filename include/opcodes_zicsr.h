#pragma once
#include "common.h"
#include "instruction.h"
#include "csrs.h"
#include "cpu.h"

#define OPCODES_ZICSR   0b1110011
#define CSRRW           0b001
#define CSRRS           0b010
#define CSRRC           0b011
#define CSRRWI          0b101
#define CSRRSI          0b110
#define CSRRCI          0b111

bool opcodes_zicsr(CPU& cpu, const Instruction& instruction);

std::optional<u64> read_csr(CPU& cpu, const u16 address);
[[nodiscard]] bool write_csr(CPU& cpu, const u64 value, const u16 address);

void csrrw  (CPU& cpu, const Instruction& instruction);
void csrrc  (CPU& cpu, const Instruction& instruction);
void csrrs  (CPU& cpu, const Instruction& instruction);
void csrrwi (CPU& cpu, const Instruction& instruction);
void csrrsi (CPU& cpu, const Instruction& instruction);
void csrrci (CPU& cpu, const Instruction& instruction);