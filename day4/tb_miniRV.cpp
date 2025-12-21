#include <stdlib.h>
#include <random>
#include <bitset>
#include <ctime>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "VminiRV.h"
#include "VminiRV__Dpi.h"
#include "gm.cpp"

int32_t sar32(uint32_t u, unsigned shift) {
  assert(shift < 32);

  if (shift == 0) return (u & 0x80000000u) ? -int32_t((~u) + 1u) : int32_t(u);

  uint32_t shifted = u >> shift;      
  if (u & 0x80000000u) {
    shifted |= (~0u << (32 - shift)); 
  }
  return static_cast<int32_t>(shifted);
}

void clock_tick(Trigger* clock) {
  clock->prev = clock->curr;
  clock->curr ^= 1;
}

void reset_dut(VminiRV* dut) {
  dut->reset = 1;
  dut->eval();
  dut->reset = 0;
  dut->eval();
  dut->clk = 0;
}

void reset_gm(miniRV* gm, Trigger* clock, Trigger* reset) {
  cpu_reset(gm);
  mem_reset(gm->ram, RAM_SIZE);
  clock->prev = 0;
  clock->curr = 0;
  reset->prev = 0;
  reset->curr = 0;
}

bool compare_reg(uint64_t sim_time, const char* name, uint32_t dut_v, uint32_t gm_v) {
  if (dut_v != gm_v) {
  std::cout << "Test Failed at time " << sim_time << ". " << name << " mismatch: dut_v = " << dut_v << " vs gm_v = " << gm_v << std::endl;
    return false;
  }
  return true;
}

bool compare(VminiRV* dut, miniRV* gm, uint64_t sim_time) {
  bool result = true;
  result &= compare_reg(sim_time, "PC", dut->pc, gm->pc.v);
  for (uint32_t i = 0; i < N_REGS; i++) {
    char digit0 = i%10 + '0';
    char digit1 = i/10 + '0';
    char name[] = {'R', digit1, digit0, '\0'};
    result &= compare_reg(sim_time, name, dut->regs_out[i], gm->regs[i].v);
  }
  return result;
}

uint64_t hash_uint64_t(uint64_t x) {
  x *= 0xff51afd7ed558ccd;
  x ^= x >> 32;
  return x;
}

uint32_t random_range(std::mt19937* gen, uint32_t ge, uint32_t lt) {
  std::uniform_int_distribution<uint32_t> dist(0, (1U << lt) - 1);

  return dist(*gen) % lt + ge;
}

uint32_t random_bits(std::mt19937* gen, uint32_t n) {
  std::uniform_int_distribution<uint32_t> dist(0, (1U << n) - 1);
  return dist(*gen);
}

inst_size_t random_instruction(std::mt19937* gen) {
  uint32_t inst_id = random_range(gen, 0, 8);
  inst_size_t inst = {};
  switch (inst_id) {
    case 0: { // ADDI
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = addi(imm, rs1, rd);
    } break;
    case 1: { // JALR
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      // inst = addi(imm, rs1, rd);
      inst = jalr(imm, rs1, rd);
    } break;
    case 2: { // ADD
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = add(rs2, rs1, rd);
    } break;
    case 3: { // LUI
      uint32_t imm = random_bits(gen, 20);
      uint32_t rd  = random_bits(gen, 4);
      inst = lui(imm, rd);
    } break;
    case 4: { // LW
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lw(imm, rs1, rd);
    } break;
    case 5: { // LB
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lbu(imm, rs1, rd);
    } break;
    case 6: { // SW
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1  = random_bits(gen, 4);
      inst = sw(imm, rs2, rs1);
    } break;
    case 7: { // SB
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1  = random_bits(gen, 4);
      inst = sb(imm, rs2, rs1);
    } break;
  }
  return inst;
}

void print_instruction(inst_size_t inst) {
  Dec_out dec = dec_eval(inst);
  switch (dec.opcode.v) {
    case OPCODE_ADDI: {
      printf("addi imm=%i\t rs1=r%u\t rd=r%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
    } break;
    case OPCODE_JALR: {
      printf("jalr imm=%i\t rs1=r%u\t rd=r%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
    } break;
    case OPCODE_ADD: { // ADD
      printf("add  rs2=r%u\t rs1=r%u\t rd=r%u\n", dec.reg_src2.v, dec.reg_src1.v, dec.reg_dest.v);
    } break;
    case OPCODE_LUI: { // LUI
      uint32_t v = sar32(dec.imm.v, 12);
      printf("lui  imm=%i\t rd=r%u\n", v, dec.reg_dest.v);
    } break;
    case OPCODE_LW: {
      if (dec.funct3.v == FUNCT3_LW) { // LW
        printf("lw   imm=%i\t rs1=r%u\t rd=r%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
      }
      else if (dec.funct3.v == FUNCT3_LBU) { // LBU
        printf("lbu  imm=%i\t rs1=r%u\t rd=r%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
      }
      else {
        printf("not implemented:%u\n", inst);
      }
    } break;
    case OPCODE_SW: {
      if (dec.funct3.v == FUNCT3_SW) { // SW
        printf("sw   imm=%i\t rs2=r%u\t rs1=r%u\n", dec.imm.v, dec.reg_src2.v, dec.reg_src1.v);
      }
      else if (dec.funct3.v == FUNCT3_SB) { // LBU
        printf("sb   imm=%i\t rs2=r%u\t rs1=r%u\n", dec.imm.v, dec.reg_src2.v, dec.reg_src1.v);
      }
      else {
        printf("not implemented:%u\n", inst);
      }
    } break;
    default: { // NOT IMPLEMENTED
      printf("not implemented:%u\n", inst);
    } break;
  }
}

int main(int argc, char** argv, char** env) {
  VminiRV *dut = new VminiRV;
  miniRV *gm = new miniRV;
  uint32_t n_inst = 200;
  inst_size_t* insts = new inst_size_t[n_inst];
  bool test_not_failed = true;
  uint64_t tests_passed = 0;
  uint64_t max_sim_time = 500;
  uint64_t max_tests = 1000;
  // uint64_t seed = hash_uint64_t(std::time(0));
  uint64_t seed = 11912696108987925668;
  do {
    printf("======== SEED:%lu =========\n", seed);
    std::random_device rd;
    std::mt19937 gen(rd());
    gen.seed(seed);
    for (uint32_t i = 0; i < n_inst; i++) {
      inst_size_t inst = random_instruction(&gen);
      insts[i] = inst;
      print_instruction(inst);
    }
    // insts[0] = addi(-520, 6, 1);
    // insts[1] = sw(1000, 1, 0);
    // insts[2] = lw(1000, 0, 2);
    // n_inst = 3;
    for (uint32_t i = 0; i < n_inst; i++) {
      print_instruction(insts[i]);
    }

    Trigger clock = {};
    Trigger reset = {};
    dut->clk = 0;
    dut->rom_wen = 1;
    for (uint32_t i = 0; i < n_inst; i++) {
      dut->eval();
      dut->rom_wdata = insts[i].v;
      dut->rom_addr = i*4;
      dut->clk ^= 1;
      dut->eval();
      dut->clk ^= 1;

      // NOTE: Golden Model loads the same instructions
      mem_write(gm->rom, clock, reset, {1}, {1, 1, 1, 1}, {i*4}, insts[i], ROM_SIZE);
      clock_tick(&clock);
      mem_write(gm->rom, clock, reset, {1}, {1, 1, 1, 1}, {i*4}, insts[i], ROM_SIZE);
      clock_tick(&clock);
    }
    dut->rom_wen = 0;
    reset_dut(dut);
    reset_gm(gm, &clock, &reset);
    for (uint64_t t = 0; t < max_sim_time && gm->pc.v < n_inst * 4; t++) {
      dut->eval();
      inst_size_t inst = mem_read(gm->rom, gm->pc, ROM_SIZE);
      cpu_eval(gm, clock, reset);
      test_not_failed &= compare(dut, gm, t);
      dut->clk ^= 1;
      clock_tick(&clock);
      if(!test_not_failed) {
        printf("[%u] %u inst: ", t, inst.v);
        print_instruction(inst);
        break;
      }
    }
    reset_dut(dut);
    reset_gm(gm, &clock, &reset);
    mem_reset(gm->ram, ROM_SIZE);

    seed = hash_uint64_t(seed);
    if (test_not_failed) {
      tests_passed++;
    }
  } while (test_not_failed && tests_passed < max_tests);

  std::cout << "Tests results:\n" << tests_passed << " / " << max_tests << " have passed\n";

  delete insts;
  delete dut;
  delete gm;
  exit(EXIT_SUCCESS);
}


