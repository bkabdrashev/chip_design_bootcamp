#include <cstdint>
#include <assert.h>

#define ERROR_NOT_IMPLEMENTED (1010)
#define ERROR_INVALID_RANGE   (2020)
#define OPCODE_BITS 7u
#define FUNCT3_BITS 3u
#define REG_INDEX_BITS 5u
#define ADDR_BITS 32u
#define REG_BITS 32u
#define BYTE 8u
#define INST_BITS 32u
#define ROM_SIZE (1u << 16) // 64K
#define RAM_SIZE (1u << 16) // 64K
#define N_REGS 16u

#define OPCODE_ADDI  (0b0010011)
#define FUNCT3_ADDI  (0b000)
#define OPCODE_JALR  (0b1100111)
#define FUNCT3_JALR  (0b000)
#define OPCODE_ADD   (0b0110011)
#define FUNCT3_ADD   (0b000)
#define OPCODE_LUI   (0b0110111)
#define FUNCT3_LUI   (0b000)
#define OPCODE_LW    (0b0000011)
#define OPCODE_LBU   (OPCODE_LW)
#define FUNCT3_LW    (0b010)
#define FUNCT3_LBU   (0b100)
#define OPCODE_SW    (0b0100011)
#define OPCODE_SB    (OPCODE_SW)
#define FUNCT3_SW    (0b010)
#define FUNCT3_SB    (0b000)
#define INST(name) (((FUNCT3_##name) << OPCODE_BITS) | (OPCODE_##name))

struct bit {
  uint32_t v : 1;
};
struct bit4 {
  union {
    struct {
      bit a;
      bit b;
      bit c;
      bit d;
    };
    bit bits[4];
  };
};
struct Trigger {
  uint32_t prev : 1;
  uint32_t curr : 1;
};
struct opcode_size_t {
  uint32_t v : OPCODE_BITS;
};
struct funct3_size_t {
  uint32_t v : OPCODE_BITS;
};
struct addr_size_t {
  uint32_t v : ADDR_BITS;
};
struct reg_index_t {
  uint32_t v : REG_BITS;
};
struct reg_size_t {
  uint32_t v : REG_BITS;
};
struct byte {
  uint8_t v : BYTE;
};

typedef reg_size_t inst_size_t;

bit ZERO_BIT = {};
reg_index_t REG_0 = {};
reg_size_t REG_VALUE_0 = {};

struct miniRV {
  addr_size_t pc;
  reg_size_t regs[N_REGS];
  byte rom[ROM_SIZE];
  byte ram[RAM_SIZE];
};

uint32_t take_bit(uint32_t bits, uint32_t pos) {
  if (pos >= REG_BITS) {
    assert(0);
    return ERROR_INVALID_RANGE;
  }

  uint32_t mask = (1u << pos);
  return (bits & mask) >> pos;
}

uint32_t take_bits_range(uint32_t bits, uint32_t from, uint32_t to) {
  if (to >= REG_BITS || from >= REG_BITS || from > to) {
    assert(0);
    return ERROR_INVALID_RANGE;
  }

  uint32_t mask = ((1u << (to - from + 1)) - 1u) << from;
  return (bits & mask) >> from;
}

bool is_positive_edge(Trigger trigger) {
  if (!trigger.prev && trigger.curr) {
    return true;
  }
  return false;
}

bool is_negative_edge(Trigger trigger) {
  if (trigger.prev && !trigger.curr) {
    return true;
  }
  return false;
}

reg_size_t alu_eval(opcode_size_t opcode, reg_size_t rdata1, reg_size_t rdata2, reg_size_t imm) {
  reg_size_t result = {};
  if (opcode.v == OPCODE_ADDI) {
    // ADDI
    result.v = rdata1.v + imm.v;
  }
  else if (opcode.v == OPCODE_ADD) {
    result.v = rdata1.v + rdata2.v;
  }
  else {
    result.v = ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

void mem_reset(byte* mem, uint64_t max_size) {
  for (uint64_t i = 0; i < max_size; i++) {
    mem[i].v = 0;
  }
}

inst_size_t mem_read(byte* mem, addr_size_t addr, uint64_t max_size) {
  inst_size_t result = {0};
  if (addr.v < max_size) {
    result.v = 
      mem[addr.v+3].v << 24 | mem[addr.v+2].v << 16 |
      mem[addr.v+1].v <<  8 | mem[addr.v+0].v <<  0 ;
  }
  else {
    // printf("WARNING: try to access ROM out bounds\n");
  }
  return result;
}

void mem_write(byte* mem, Trigger clock, Trigger reset, bit write_enable, bit4 write_enable_bytes, addr_size_t addr, reg_size_t write_data, uint64_t max_addr_size) {
  if (is_positive_edge(reset)) {
    for (uint32_t i = 0; i < max_addr_size; i++) {
      mem[i].v = 0;
    }
  }
  else if (is_positive_edge(clock)) {
    if (write_enable.v && addr.v < max_addr_size) {
      if (write_enable_bytes.bits[0].v) {
        mem[addr.v + 0].v = (write_data.v & (0xff << 0)) >> 0;
      }
      if (write_enable_bytes.bits[1].v) {
        mem[addr.v + 1].v = (write_data.v & (0xff << 8)) >> 8;
      }
      if (write_enable_bytes.bits[2].v) {
        mem[addr.v + 2].v = (write_data.v & (0xff << 16)) >> 16;
      }
      if (write_enable_bytes.bits[3].v) {
        mem[addr.v + 3].v = (write_data.v & (0xff << 24)) >> 24;
      }
    }
  }
}

void pc_write(miniRV* cpu, Trigger clock, Trigger reset, addr_size_t in_addr, bit is_jump) {
  if (is_positive_edge(reset)) {
    cpu->pc.v = 0;
  }
  else if (is_positive_edge(clock)) {
    if (is_jump.v) {
      cpu->pc.v = in_addr.v;
    }
    else {
      cpu->pc.v += 4;
    }
  }
}

struct RF_out {
  reg_size_t rdata1;
  reg_size_t rdata2;
  reg_size_t regs[N_REGS];
};

RF_out rf_read(miniRV* cpu, reg_index_t reg_src1, reg_index_t reg_src2) {
  RF_out out = {};
  out.rdata1.v = cpu->regs[reg_src1.v].v;
  out.rdata2.v = cpu->regs[reg_src2.v].v;
  for (int i = 0; i < N_REGS; i++) {
    out.regs[i].v = cpu->regs[i].v;
  }
  return out;
}

void rf_write(miniRV* cpu, Trigger clock, Trigger reset, bit write_enable, reg_index_t reg_dest, reg_size_t write_data) {
  if (is_positive_edge(reset)) {
    for (uint32_t i = 0; i < N_REGS; i++) {
      cpu->regs[i].v = 0;
    }
  }
  else if (is_positive_edge(clock)) {
    if (write_enable.v && reg_dest.v != 0) {
      cpu->regs[reg_dest.v] = write_data;
    }
  }
}

struct Dec_out {
  reg_index_t reg_dest;
  reg_index_t reg_src1;
  reg_index_t reg_src2;
  reg_size_t imm;
  opcode_size_t opcode;
  funct3_size_t funct3;
  bit write_enable;
};

Dec_out dec_eval(inst_size_t inst) {
  Dec_out out = {};
  out.opcode.v = take_bits_range(inst.v, 0, 6);
  out.reg_dest.v = take_bits_range(inst.v, 7, 11);
  out.funct3.v = take_bits_range(inst.v, 12, 14);
  out.reg_src1.v = take_bits_range(inst.v, 15, 19);
  out.reg_src2.v = take_bits_range(inst.v, 20, 24);
  bit sign = {.v=take_bit(inst.v, 31)};
  if (out.opcode.v == OPCODE_ADDI) {
    // ADDI
    if (sign.v) out.imm.v = (-1 << 12) | take_bits_range(inst.v, 20, 31);
    else        out.imm.v = ( 0 << 12) | take_bits_range(inst.v, 20, 31);
    out.write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_JALR) {
    // JALR
    if (sign.v) out.imm.v = (-1 << 12) | take_bits_range(inst.v, 20, 31);
    else        out.imm.v = ( 0 << 12) | take_bits_range(inst.v, 20, 31);
    out.write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_ADD) {
    // ADD
    if (sign.v) out.imm.v = (-1 << 20) | take_bits_range(inst.v, 12, 31);
    else        out.imm.v = ( 0 << 20) | take_bits_range(inst.v, 12, 31);
    out.write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_LUI) {
    // LUI
    out.imm.v = take_bits_range(inst.v, 12, 31) << 12;
    out.write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_LW) {
    // LW, LBU
    if (sign.v) out.imm.v = (-1 << 12) | take_bits_range(inst.v, 20, 31);
    else        out.imm.v = ( 0 << 12) | take_bits_range(inst.v, 20, 31);
    out.write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_SW) {
    // SW, SB
    uint32_t top_imm = take_bits_range(inst.v, 25, 31);
    uint32_t bot_imm = take_bits_range(inst.v, 7, 11);
    top_imm <<= 5;
    if (sign.v) out.imm.v = (-1 << 12) | top_imm | bot_imm;
    else        out.imm.v = ( 0 << 12) | top_imm | bot_imm;
    out.write_enable.v = 0;
  }
  else {
    out.imm.v = 0;
    out.write_enable.v = 0;
  }
  return out;
}

void cpu_eval(miniRV* cpu, Trigger clock, Trigger reset) {
  inst_size_t inst = mem_read(cpu->rom, cpu->pc, ROM_SIZE);
  Dec_out dec_out = dec_eval(inst);
  RF_out rf_out = rf_read(cpu, dec_out.reg_src1, dec_out.reg_src2);
  reg_size_t alu_out = alu_eval(dec_out.opcode, rf_out.rdata1, rf_out.rdata2, dec_out.imm);

  addr_size_t in_addr = {};
  bit is_jump = {};
  reg_size_t reg_write_data = {};

  reg_size_t ram_write_data = {};
  bit ram_write_enable = {};
  bit4 ram_write_enable_bytes = {};
  addr_size_t ram_addr = {};
  if (dec_out.opcode.v == OPCODE_ADDI) {
    // ADDI
    ram_write_enable.v = 0;
    ram_addr.v = 0;
    ram_write_data.v = 0;
    reg_write_data = alu_out;
    ram_write_enable_bytes = {0, 0, 0, 0};
    in_addr.v = 0;
    is_jump.v = 0;
  }
  else if (dec_out.opcode.v == OPCODE_JALR) {
    // JALR
    ram_write_enable.v = 0;
    ram_addr.v = 0;
    ram_write_data.v = 0;
    ram_write_enable_bytes = {0, 0, 0, 0};
    reg_write_data.v= cpu->pc.v + 4;
    in_addr.v = (rf_out.rdata1.v + dec_out.imm.v) & ~3;
    is_jump.v = 1;
  }
  else if (dec_out.opcode.v == OPCODE_ADD) {
    // ADD
    ram_write_enable.v = 0;
    ram_addr.v = 0;
    ram_write_data.v = 0;
    ram_write_enable_bytes = {0, 0, 0, 0};
    reg_write_data = alu_out;
    in_addr.v = 0;
    is_jump.v = 0;
  }
  else if (dec_out.opcode.v == OPCODE_LUI) {
    // LUI
    ram_write_enable.v = 0;
    ram_addr.v = 0;
    ram_write_data.v = 0;
    ram_write_enable_bytes = {0, 0, 0, 0};
    reg_write_data.v = dec_out.imm.v;
    in_addr.v = 0;
    is_jump.v = 0;
  }
  else if (dec_out.opcode.v == OPCODE_LW && dec_out.funct3.v == FUNCT3_LW) {
    // LW
    ram_write_enable.v = 0;
    ram_addr.v = rf_out.rdata1.v + dec_out.imm.v;
    ram_write_data.v = 0;
    ram_write_enable_bytes = {0, 0, 0, 0};
    reg_write_data = mem_read(cpu->ram, ram_addr, RAM_SIZE);
    in_addr.v = 0;
    is_jump.v = 0;
  }
  else if (dec_out.opcode.v == OPCODE_LW && dec_out.funct3.v == FUNCT3_LBU) {
    // LBU
    ram_write_enable.v = 0;
    ram_addr.v = rf_out.rdata1.v + dec_out.imm.v;
    ram_write_data.v = 0;
    ram_write_enable_bytes = {0, 0, 0, 0};
    reg_write_data.v = mem_read(cpu->ram, ram_addr, RAM_SIZE).v & 0xff;
    in_addr.v = 0;
    is_jump.v = 0;
  }
  else if (dec_out.opcode.v == OPCODE_SW) {
    // SW
    ram_write_enable.v = 1;
    ram_addr.v = rf_out.rdata1.v + dec_out.imm.v;
    ram_write_data = rf_out.rdata2;
    if (dec_out.funct3.v == FUNCT3_SW) {
      ram_write_enable_bytes = {1, 1, 1, 1};
    }
    else if (dec_out.funct3.v == FUNCT3_SB) {
      ram_write_enable_bytes = {1, 0, 0, 0};
    }
    reg_write_data.v = 0;
    in_addr.v = 0;
    is_jump.v = 0;
  }
  else {
    printf("WARNING: NOT IMPLEMENTED\n");
    ram_write_enable.v = 0;
    ram_addr.v = 0;
    ram_write_data.v = 0;
    ram_write_enable_bytes = {0, 0, 0, 0};
    reg_write_data.v = 0;
    in_addr.v = 0;
    is_jump.v = 0;
  }

  rf_write(cpu, clock, reset, dec_out.write_enable, dec_out.reg_dest, reg_write_data);
  mem_write(cpu->ram, clock, reset, ram_write_enable, ram_write_enable_bytes, ram_addr, ram_write_data, RAM_SIZE);
  pc_write(cpu, clock, reset, in_addr, is_jump);
}

void cpu_reset(miniRV* cpu) {
  cpu->pc.v = 0;
  for (uint32_t i = 0; i < N_REGS; i++) {
    cpu->regs[i].v = 0;
  }
}

inst_size_t addi(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_ADDI << 12) | (reg_dest << 7) | OPCODE_ADDI;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t li(uint32_t imm, uint32_t reg_dest) {
  return addi(imm, 0, reg_dest);
}

inst_size_t jalr(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_JALR << 12) | (reg_dest << 7) | OPCODE_JALR;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t add(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_ADD << 12) | (reg_dest << 7) | OPCODE_ADD;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t lui(uint32_t imm, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 12) | (reg_dest << 7) | OPCODE_LUI;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t lw(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LW << 12) | (reg_dest << 7) | OPCODE_LW;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t lbu(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LBU << 12) | (reg_dest << 7) | OPCODE_LW;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t sw(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1) {
  uint32_t top_imm = ((-1 << 5) & imm) >> 5;
  uint32_t bot_imm = 0b11111 & imm;
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SW << 12) | (bot_imm << 7) | OPCODE_SW;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t sb(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1) {
  uint32_t top_imm = imm << 5;
  uint32_t bot_imm = 0b11111 & imm;
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SB << 12) | (bot_imm << 7) | OPCODE_SW;
  inst_size_t result = { inst_u32 };
  return result;
}
