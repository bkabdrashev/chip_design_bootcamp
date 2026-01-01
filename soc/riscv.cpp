#include "stdio.h"

#define INITIAL_PC  (0x3000'0000)
#define N_REGS      (16)
#define OP_ADD  (0b0000)
#define OP_SUB  (0b1000) 
#define OP_SLL  (0b0001) 
#define OP_SLT  (0b0010) 
#define OP_SLTU (0b0011) 
#define OP_XOR  (0b0100) 
#define OP_SRL  (0b0101) 
#define OP_SRA  (0b1101) 
#define OP_OR   (0b0110) 
#define OP_AND  (0b0111) 
#define FUNCT3_SR   (0b101)

#define INST_UNDEFINED (0b0000)
#define INST_LOAD_BYTE (0b1000)
#define INST_LOAD_HALF (0b1001)
#define INST_LOAD_WORD (0b1010)
#define INST_STORE     (0b1011)
#define INST_IMM       (0b1100)
#define INST_REG       (0b1101)
#define INST_UPP       (0b1110)
#define INST_JUMP      (0b1111)

#define FUNCT3_BYTE         (0b000)
#define FUNCT3_HALF         (0b001)
#define FUNCT3_WORD         (0b010)
#define FUNCT3_BYTE_UNSIGN  (0b100)
#define FUNCT3_HALF_UNSIGN  (0b101)

#define OPCODE_LUI          (0b0110111)
#define OPCODE_AUIPC        (0b0010111)
#define OPCODE_JAL          (0b1101111)
#define OPCODE_JALR         (0b1100111)
#define OPCODE_LOAD         (0b0000011)
#define OPCODE_STORE        (0b0100011)
#define OPCODE_CALC_IMM     (0b0010011)
#define OPCODE_CALC_REG     (0b0110011)
#define OPCODE_ENV          (0b1110011)

#define ERROR_NOT_IMPLEMENTED (1010)
#define ERROR_INVALID_RANGE   (2020)

#define OPCODE_ADDI  (0b0010011)
#define FUNCT3_ADDI  (0b000)
#define FUNCT3_SLLI  (0b001)
#define FUNCT3_SRLI  (0b101)
#define FUNCT7_SRLI  (0b0000000)
#define FUNCT7_SRAI  (0b0100000)

#define OPCODE_JALR  (0b1100111)
#define FUNCT3_JALR  (0b000)
#define OPCODE_ADD   (0b0110011)
#define FUNCT3_ADD   (0b000)
#define OPCODE_LUI   (0b0110111)
#define FUNCT3_LUI   (0b000)
#define OPCODE_LW    (0b0000011)
#define FUNCT3_LW    (0b010)
#define FUNCT3_LBU   (0b100)
#define OPCODE_SW    (0b0100011)
#define FUNCT3_SW    (0b010)
#define FUNCT3_SB    (0b000)
#define OPCODE_EBREAK (0b1110011)

#define REG_SP (2)
#define REG_T0 (5)
#define REG_T1 (6)
#define REG_T2 (7)
#define REG_A0 (10)

int32_t sar32(uint32_t u, unsigned shift) {
  assert(shift < 32);

  if (shift == 0) return (u & 0x80000000u) ? -int32_t((~u) + 1u) : int32_t(u);

  uint32_t shifted = u >> shift;      
  if (u & 0x80000000u) {
    shifted |= (~0u << (32 - shift)); 
  }
  return static_cast<int32_t>(shifted);
}

uint32_t take_bit(uint32_t bits, uint32_t pos) {
  if (pos >= 32) {
    assert(0);
    return ERROR_INVALID_RANGE;
  }

  uint32_t mask = (1u << pos);
  return (bits & mask) >> pos;
}

uint32_t take_bits_range(uint32_t bits, uint32_t from, uint32_t to) {
  if (to >= 32 || from >= 32 || from > to) {
    assert(0);
    return ERROR_INVALID_RANGE;
  }

  uint32_t mask = ((1u << (to - from + 1)) - 1u) << from;
  return (bits & mask) >> from;
}

uint32_t addi(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_ADDI << 12) | (reg_dest << 7) | OPCODE_ADDI;
  return inst_u32;
}

uint32_t slli(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_SLLI << 12) | (reg_dest << 7) | OPCODE_ADDI;
  return inst_u32;
}

uint32_t srli(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (FUNCT7_SRLI << 25) | (imm << 20) | (reg_src1 << 15) | (FUNCT3_SRLI << 12) | (reg_dest << 7) | OPCODE_ADDI;
  return inst_u32;
}

uint32_t srai(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (FUNCT7_SRAI << 25) | (imm << 20) | (reg_src1 << 15) | (FUNCT3_SRLI << 12) | (reg_dest << 7) | OPCODE_ADDI;
  return inst_u32;
}

uint32_t li(uint32_t imm, uint32_t reg_dest) {
  return addi(imm, 0, reg_dest);
}

uint32_t jalr(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_JALR << 12) | (reg_dest << 7) | OPCODE_JALR;
  return inst_u32;
}

uint32_t add(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_ADD << 12) | (reg_dest << 7) | OPCODE_ADD;
  return inst_u32;
}

uint32_t lui(uint32_t imm, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 12) | (reg_dest << 7) | OPCODE_LUI;
  return inst_u32;
}

uint32_t lw(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LW << 12) | (reg_dest << 7) | OPCODE_LW;
  return inst_u32;
}

uint32_t lbu(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LBU << 12) | (reg_dest << 7) | OPCODE_LW;
  return inst_u32;
}

uint32_t sw(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1) {
  uint32_t top_imm = imm >> 5;
  uint32_t bot_imm = 0b11111 & imm;
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SW << 12) | (bot_imm << 7) | OPCODE_SW;
  return inst_u32;
}

uint32_t sb(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1) {
  uint32_t top_imm = imm >> 5;
  uint32_t bot_imm = 0b11111 & imm;
  printf("top %u, bot: %u\n", top_imm, bot_imm);
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SB << 12) | (bot_imm << 7) | OPCODE_SW;
  return inst_u32;
}

uint32_t ebreak() {
  uint32_t inst_u32 = 1u << 20 | OPCODE_EBREAK;
  return inst_u32;
}

struct InstInfo {
  uint8_t reg_dest;
  uint8_t reg_src1;
  uint8_t reg_src2;
  uint32_t imm;
  uint8_t opcode;
  uint8_t funct3;
  uint8_t funct7;
  uint8_t reg_write_enable;
};

InstInfo dec_eval(uint32_t inst) {
  InstInfo out = {};
  out.opcode = take_bits_range(inst, 0, 6);
  out.reg_dest = take_bits_range(inst, 7, 11);
  out.funct3 = take_bits_range(inst, 12, 14);
  out.funct7 = take_bits_range(inst, 25, 31);
  out.reg_src1 = take_bits_range(inst, 15, 19);
  out.reg_src2 = take_bits_range(inst, 20, 24);
  uint8_t sign = take_bit(inst, 31);
  if (out.opcode == OPCODE_ADDI) {
    // ADDI, SLLI, SRLI, SARLI
    if (sign) out.imm = (~0u << 12) | take_bits_range(inst, 20, 31);
    else      out.imm = ( 0u << 12) | take_bits_range(inst, 20, 31);
    if (out.funct3 == FUNCT3_SLLI || out.funct3 == FUNCT3_SRLI) {
      out.imm &= 0b11111;
    }
    out.reg_write_enable = 1;
  }
  else if (out.opcode == OPCODE_JALR) {
    // JALR
    if (sign) out.imm = (~0u << 12) | take_bits_range(inst, 20, 31);
    else      out.imm = ( 0u << 12) | take_bits_range(inst, 20, 31);
    out.reg_write_enable = 1;
  }
  else if (out.opcode == OPCODE_ADD) {
    // ADD
    if (sign) out.imm = (~0u << 20) | take_bits_range(inst, 12, 31);
    else      out.imm = ( 0u << 20) | take_bits_range(inst, 12, 31);
    out.reg_write_enable = 1;
  }
  else if (out.opcode == OPCODE_LUI) {
    // LUI
    out.imm = take_bits_range(inst, 12, 31) << 12;
    out.reg_write_enable = 1;
  }
  else if (out.opcode == OPCODE_LW) {
    // LW, LBU
    if (sign) out.imm = (~0u << 12) | take_bits_range(inst, 20, 31);
    else      out.imm = ( 0u << 12) | take_bits_range(inst, 20, 31);
    out.reg_write_enable = 1;
  }
  else if (out.opcode == OPCODE_SW) {
    // SW, SB
    uint32_t top_imm = take_bits_range(inst, 25, 31);
    uint32_t bot_imm = take_bits_range(inst, 7, 11);
    top_imm <<= 5;
    if (sign) out.imm = (~0u << 12) | top_imm | bot_imm;
    else      out.imm = ( 0u << 12) | top_imm | bot_imm;
    out.reg_write_enable = 0;
  }
  else if (out.opcode == OPCODE_EBREAK) {
    // EBREAK
    out.imm = take_bits_range(inst, 20, 31);
    out.reg_write_enable = 0;
  }
  else {
    out.imm = 0;
    out.reg_write_enable = 0;
  }
  return out;
}

void print_instruction(uint32_t inst) {
  InstInfo dec = dec_eval(inst);
  switch (dec.opcode) {
    case OPCODE_ADDI: {
      if (dec.funct3 == FUNCT3_ADDI) { // ADDI
        printf("addi imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm, dec.reg_src1, dec.reg_dest);
      }
      else if (dec.funct3 == FUNCT3_SLLI) { // SLLI
         printf("srli imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm, dec.reg_src1, dec.reg_dest);
      }
      else if (dec.funct3 == FUNCT3_SRLI) { // SRLI
        if (dec.funct7 == FUNCT7_SRLI) {
          printf("srli imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm, dec.reg_src1, dec.reg_dest);
        }
        else if (dec.funct7 == FUNCT7_SRAI) {
          printf("srai imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm, dec.reg_src1, dec.reg_dest);
        }
        else {
          goto not_implemented;
        }
      }
      else {
        goto not_implemented;
      }

    } break;
    case OPCODE_JALR: {
      printf("jalr imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm, dec.reg_src1, dec.reg_dest);
    } break;
    case OPCODE_ADD: { // ADD
      printf("add  rs2=x%u\t rs1=x%u\t rd=x%u\n", dec.reg_src2, dec.reg_src1, dec.reg_dest);
    } break;
    case OPCODE_LUI: { // LUI
      int32_t v = sar32(dec.imm, 12);
      printf("lui  imm=%i\t rd=x%u\n", v, dec.reg_dest);
    } break;
    case OPCODE_LW: {
      if (dec.funct3 == FUNCT3_LW) { // LW
        printf("lw   imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm, dec.reg_src1, dec.reg_dest);
      }
      else if (dec.funct3 == FUNCT3_LBU) { // LBU
        printf("lbu  imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm, dec.reg_src1, dec.reg_dest);
      }
      else {
        printf("not implemented:%u\n", inst);
      }
    } break;
    case OPCODE_SW: {
      if (dec.funct3 == FUNCT3_SW) { // SW
        printf("sw   imm=%i\t rs2=x%u\t rs1=x%u\n", dec.imm, dec.reg_src2, dec.reg_src1);
      }
      else if (dec.funct3 == FUNCT3_SB) { // LBU
        printf("sb   imm=%i\t rs2=x%u\t rs1=x%u\n", dec.imm, dec.reg_src2, dec.reg_src1);
      }
      else {
        printf("not implemented:%u\n", inst);
      }
    } break;
    case OPCODE_EBREAK: {
      // EBREAK
      printf("ebreak\n");
    } break;

    default: { // NOT IMPLEMENTED
      not_implemented:
      printf("GM WARNING: not implemented:0x%x\n", inst);
    } break;
  }
}

