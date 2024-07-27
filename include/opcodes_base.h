#pragma once
#include "cpu.h"
#include "instruction.h"

/*
    https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
*/

#define OPCODES_SHIFT_RIGHT 0x5

#define OPCODES_BASE_R_TYPE 0x33
#define ADD     0x0
#define SUB     0x20 // funct7; funct3 same as ADD
#define XOR     0x4
#define OR      0x6
#define AND     0x7
#define SLL     0x1
#define SRL     0x0  // funct7; funct3 same as SRA
#define SRA     0x20 // funct7; funct3 same as SRL
#define SLT     0x2
#define SLTU    0x3

#define OPCODES_BASE_I_TYPE 0x13
#define ADDI    0x0
#define XORI    0x4
#define ORI     0x6
#define ANDI    0x7
#define SLLI    0x1
#define SRLI    0x00 // funct7; funct3 (0x5) same as srli
#define SRAI    0x20 // funct7; funct3 (0x5) same as srai
#define SLTI    0x2
#define SLTIU   0x3

#define OPCODES_BASE_LOAD_TYPE 0x3
#define LB      0x0
#define LH      0x1
#define LW      0x2
#define LBU     0x4
#define LHU     0x5

#define OPCODES_BASE_S_TYPE 0x23
#define SB      0x0
#define SH      0x1
#define SW      0x2

#define OPCODES_BASE_B_TYPE 0x63
#define BEQ     0x0
#define BNE     0x1
#define BLT     0x4
#define BGE     0x5
#define BLTU    0x6
#define BGEU    0x7

#define JAL     0b1101111
#define JALR    0b1100111

#define LUI     0b0110111
#define AUIPC   0b0010111

#define OPCODES_BASE_SYSTEM 0x73
#define ECALL               0x0
#define EBREAK              0x1
#define URET                0x2
#define SRET                0x8
#define MRET                0x18
#define WFI                 0x8
#define SFENCE_VMA          0x9
#define HFENCE_BVMA         0x11
#define HFENCE_GVMA         0x51

#define OPCODES_BASE_FENCE  0xf
#define FENCE               0x0
#define FENCE_I             0x1

// --- RV64I-specific ---

#define LWU     0b110
#define LD      0b011
#define SD      0b011

#define OPCODES_BASE_I_TYPE_32 0x1b
#define ADDIW   0b000
#define SLLIW   0b001
#define SRLIW   0x000 // funct7; funct3 (0x5) same as sraiw
#define SRAIW   0x020 // funct7; funct3 (0x5) same as srliw

#define OPCODES_BASE_R_TYPE_32 0x3b
#define ADDW    0b000 // funct7; funct3 (0x0) same as subw
#define SUBW    0x020 // funct7; funct3 (0x0) same as addw
#define SLLW    0b001
#define SRLW    0x000 // funct7; funct3 (0x5) same as sraiw
#define SRAW    0x020 // funct7; funct3 (0x5) same as srliw

bool opcodes_base(CPU& cpu, const Instruction instruction);

void add        (CPU& cpu, const Instruction instruction);
void sub        (CPU& cpu, const Instruction instruction);
void _xor       (CPU& cpu, const Instruction instruction);
void _or        (CPU& cpu, const Instruction instruction);
void _and       (CPU& cpu, const Instruction instruction);
void sll        (CPU& cpu, const Instruction instruction);
void srl        (CPU& cpu, const Instruction instruction);
void sra        (CPU& cpu, const Instruction instruction);
void slt        (CPU& cpu, const Instruction instruction);
void sltu       (CPU& cpu, const Instruction instruction);

void addi       (CPU& cpu, const Instruction instruction);
void xori       (CPU& cpu, const Instruction instruction);
void ori        (CPU& cpu, const Instruction instruction);
void andi       (CPU& cpu, const Instruction instruction);
void slli       (CPU& cpu, const Instruction instruction);
void srli       (CPU& cpu, const Instruction instruction);
void srai       (CPU& cpu, const Instruction instruction);
void slti       (CPU& cpu, const Instruction instruction);
void sltiu      (CPU& cpu, const Instruction instruction);

void lb         (CPU& cpu, const Instruction instruction);
void lh         (CPU& cpu, const Instruction instruction);
void lw         (CPU& cpu, const Instruction instruction);
void lbu        (CPU& cpu, const Instruction instruction);
void lhu        (CPU& cpu, const Instruction instruction);

void sb         (CPU& cpu, const Instruction instruction);
void sh         (CPU& cpu, const Instruction instruction);
void sw         (CPU& cpu, const Instruction instruction);

void beq        (CPU& cpu, const Instruction instruction);
void bne        (CPU& cpu, const Instruction instruction);
void blt        (CPU& cpu, const Instruction instruction);
void bge        (CPU& cpu, const Instruction instruction);
void bltu       (CPU& cpu, const Instruction instruction);
void bgeu       (CPU& cpu, const Instruction instruction);

void jal        (CPU& cpu, const Instruction instruction);
void jalr       (CPU& cpu, const Instruction instruction);

void lui        (CPU& cpu, const Instruction instruction);
void auipc      (CPU& cpu, const Instruction instruction);

void ecall      (CPU& cpu, const Instruction instruction);
void ebreak     (CPU& cpu, const Instruction instruction);
void uret       (CPU& cpu, const Instruction instruction);
void sret       (CPU& cpu, const Instruction instruction);
void mret       (CPU& cpu, const Instruction instruction);
void wfi        (CPU& cpu, const Instruction instruction);
void sfence_vma (CPU& cpu, const Instruction instruction);

void lwu        (CPU& cpu, const Instruction instruction);
void ld         (CPU& cpu, const Instruction instruction);
void sd         (CPU& cpu, const Instruction instruction);

void addiw      (CPU& cpu, const Instruction instruction);
void slliw      (CPU& cpu, const Instruction instruction);
void srliw      (CPU& cpu, const Instruction instruction);
void sraiw      (CPU& cpu, const Instruction instruction);

void addw       (CPU& cpu, const Instruction instruction);
void subw       (CPU& cpu, const Instruction instruction);
void sllw       (CPU& cpu, const Instruction instruction);
void srlw       (CPU& cpu, const Instruction instruction);
void sraw       (CPU& cpu, const Instruction instruction);
