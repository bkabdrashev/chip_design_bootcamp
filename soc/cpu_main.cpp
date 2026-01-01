#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vcpu.h"

#include <stdio.h>   // fopen, fseek, ftell, fread, fclose, fprintf
#include <stdlib.h>  // malloc, free
#include <stdint.h>  // uint8_t
#include <stddef.h>  // size_t
#include <limits.h>  // SIZE_MAX

#include "riscv.cpp"
#include "gcpu.cpp"

int read_bin_file(const char* path, uint8_t** out_data, size_t* out_size) {
  if (!out_data || !out_size) return 0;

  *out_data = NULL;
  *out_size = 0;

  FILE* f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "Error: Could not open %s\n", path);
    return 0;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fprintf(stderr, "Error: Could not seek to end of file.\n");
    fclose(f);
    return 0;
  }

  long len = ftell(f);
  if (len < 0) { // ftell failure
    fprintf(stderr, "Error: Could not determine file size.\n");
    fclose(f);
    return 0;
  }

  if (len == 0) { // empty file is not an error
    fclose(f);
    return 1;
  }

  // Ensure it fits into size_t
  if ((unsigned long long)len > (unsigned long long)SIZE_MAX) {
    fprintf(stderr, "Error: File too large to allocate.\n");
    fclose(f);
    return 0;
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    fprintf(stderr, "Error: Could not seek to start of file.\n");
    fclose(f);
    return 0;
  }

  size_t size = (size_t)len;
  uint8_t* buf = (uint8_t*)malloc(size);
  if (!buf) {
    fprintf(stderr, "Error: Out of memory.\n");
    fclose(f);
    return 0;
  }

  size_t read = fread(buf, 1, size, f);
  fclose(f);

  if (read != size) {
    fprintf(stderr, "Error: Could not read file.\n");
    free(buf);
    return 0;
  }

  *out_data = buf;
  *out_size = size;
  return 1;
}

extern "C" void flash_init(uint8_t* data, uint32_t size);

struct TestBenchConfig {
  bool is_trace  = false;
  char* trace_file;
  bool is_cycles = false;
  bool is_bin    = false;
  char* bin_file = NULL;
  uint64_t max_cycles = 0;

  bool is_diff = false;
  bool is_random = false;
  uint64_t max_tests = 0;
  uint32_t  n_insts;
};

struct TestBench {
  bool is_trace;
  char* trace_file;
  bool is_cycles;
  bool is_bin;
  char* bin_file;
  uint64_t max_cycles;

  bool is_diff;
  bool is_random;
  uint64_t max_tests;

  VerilatedContext* contextp;
  Vcpu* vcpu;
  Gcpu* gcpu;
  VerilatedVcdC* trace;
  uint64_t cycles;

  size_t    file_size;
  uint32_t  n_insts;
  uint32_t* insts;
};

void print_all_instructions(TestBench* tb) {
  for (uint32_t i = 0; i < tb->n_insts; i++) {
    printf("[0x%x], (0x%x)\t", 4*i, tb->insts[i]);
    print_instruction(tb->insts[i]);
  }
}

void tick(TestBench* tb) {
  tb->vcpu->eval();
  if (tb->is_trace) tb->trace->dump(tb->cycles);
  tb->cycles++;
  tb->vcpu->clock ^= 1;
}

void cycle(TestBench* tb) {
  tick(tb);
  tick(tb);
}

void reset(TestBench* tb) {
  printf("[INFO] reset\n");
  tb->vcpu->reset = 1;
  tb->vcpu->clock = 0;
  for (uint64_t i = 0; i < 10; i++) {
    cycle(tb);
  }
  tb->vcpu->reset = 0;
}

void run(TestBench* tb) {
  tb->vcpu->clock = 0;
  while (1) {
    if (tb->max_cycles && tb->cycles >= tb->max_cycles) break;
    if (tb->contextp->gotFinish()) break;
    cycle(tb);
  }
  tb->vcpu->reset = 0;
}

void fetch_exec(TestBench* tb) {
  tb->vcpu->clock = 0;
  while (1) {
    if (tb->max_cycles && tb->cycles >= tb->max_cycles) break;
    if (tb->contextp->gotFinish()) break;
    if (tb->vcpu->is_done_instruction) break;
    cycle(tb);
  }
  tb->vcpu->reset = 0;
}

bool test_instructions(TestBench* tb) {
  bool is_test_success = true;
  tb->cycles = 0;
  reset(tb);
  while (1) {
    fetch_exec(tb);
    if (tb->contextp->gotFinish()) {
      printf("[INFO] vcpu ebreak\n");
      if (tb->vcpu->regs[10] != 0) {
        printf("[FAILED] test is not successful: returned %u\n", tb->vcpu->regs[10]);
        is_test_success=false;
      }
    }
    if (tb->max_cycles && tb->cycles >= tb->max_cycles) {
      printf("[FAILED] test is not successful: timeout %u/%u\n", tb->cycles, tb->max_cycles);
      is_test_success=false;
      break;
    }
  }
  printf("[INFO] finished:%u cycles\n", tb->cycles);
  return is_test_success;
}

bool test_bin(TestBench* tb) {
  bool is_test_success = true;
  return is_test_success;
}

bool test_random(TestBench* tb) {
  bool is_tests_success = true;
  return is_tests_success;
}

void simple_lw_sw_test(TestBench* tb) {
  uint32_t insts[7] = {
    lui(0x80000, REG_SP),     // 0
    li(0x12, REG_T0),         // 4
    sw( 0x4, REG_T0, REG_SP), // 8
    li(0x34, REG_T1),         // C
    sw( 0x5, REG_T1, REG_SP), // 10
    lw( 0x4, REG_SP, REG_T2), // 14
    ebreak()
  };

  tb->n_insts = 7;
  tb->insts = (uint32_t*)malloc(sizeof(uint32_t) * tb->n_insts);
  for (uint32_t i = 0; i < tb->n_insts; i++) {
    tb->insts[i] = insts[i];
  }

  reset(tb);
  run(tb);
}

static void usage(const char* prog) {
  fprintf(stderr,
    "Usage:\n"
    "  %s [diff] [trace <path>] [cycles] [max <cycles>] bin <path>\n"
    "  %s [diff] [trace <path>] [cycles] [max <cycles>] random <tests> <n_insts>\n",
    prog, prog
  );
}

static int streq(const char* a, const char* b) {
  return a && b && strcmp(a, b) == 0;
}

TestBench new_testbench(TestBenchConfig config) {
  TestBench tb = {
    .is_trace   = config.is_trace,
    .trace_file = config.trace_file,
    .is_cycles  = config.is_cycles,
    .is_bin     = config.is_bin,
    .bin_file   = config.bin_file,
    .max_cycles = config.max_cycles,
    .is_diff    = config.is_diff,
    .is_random  = config.is_random,
    .max_tests  = config.max_tests,
    .n_insts    = config.n_insts,
  };
  tb.contextp = new VerilatedContext;
  tb.vcpu  = new Vcpu;
  tb.gcpu  = new Gcpu;

  if (tb.is_trace) {
    Verilated::traceEverOn(true);
    tb.trace = new VerilatedVcdC;
    tb.vcpu->trace(tb.trace, 5);
    tb.trace->open(tb.trace_file);
  }
  return tb;
}

void delete_testbench(TestBench tb) {
  if (tb.n_insts) {
    free(tb.insts);
  }
  if (tb.is_trace) {
    tb.trace->close();
    delete tb.trace;
  }
  delete tb.vcpu;
  delete tb.gcpu;
  delete tb.contextp;
}

int main(int argc, char** argv, char** env) {
  int exit_code = EXIT_SUCCESS;

  if (argc < 2) {
    usage(argv[0]);
    exit_code = EXIT_FAILURE;
    goto exit_label;
  }
  else {
    TestBenchConfig config = {};
    int curr_arg = 1;
    while (curr_arg < argc) {
      char* mode = argv[curr_arg++];
      if (streq(mode, "trace")) {
        if (config.is_trace) {
          fprintf(stderr, "Error: second trace\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        if (curr_arg >= argc) {
          fprintf(stderr, "Error: 'trace' requires a <path>\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        config.trace_file = argv[curr_arg++];
        config.is_trace = true;
      }
      else if (streq(mode, "diff")) {
        config.is_diff = true;
      }
      else if (streq(mode, "cycles")) {
        config.is_cycles = true;
      }
      else if (streq(mode, "max")) {
        if (config.max_cycles) {
          fprintf(stderr, "Error: second max cycles\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        if (curr_arg >= argc) {
          fprintf(stderr, "Error: 'max' requires a <number>\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        config.max_cycles = atoi(argv[curr_arg++]);
      }
      else if (streq(mode, "random")) {
        if (config.is_random) {
          fprintf(stderr, "Error: second random is not supported\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        if (curr_arg+1 >= argc) {
          fprintf(stderr, "Error: 'random' requires a <number> <number>\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        config.is_random = true;
        config.max_tests = atoi(argv[curr_arg++]);
        config.n_insts   = atoi(argv[curr_arg++]);
      }
      else if (streq(mode, "bin")) {
        if (config.is_bin) {
          fprintf(stderr, "Error: second bin is not supported\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        if (curr_arg >= argc) {
          fprintf(stderr, "Error: 'bin' requires a <path>\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        config.is_bin = true;
        config.bin_file = argv[curr_arg++];
      }
      else {
        fprintf(stderr, "Error: unknown mode '%s'\n", mode);
        usage(argv[0]);
        exit_code = EXIT_FAILURE;
      }
    }
    TestBench tb = new_testbench(config);

    if (tb.is_bin) {
      bool result = test_bin(&tb);
      if (!result) exit_code = EXIT_FAILURE;
    }
    else if (tb.is_random) {
      bool result = test_random(&tb);
      if (!result) exit_code = EXIT_FAILURE;
    }
    delete_testbench(tb);
  }
  
exit_label:
  return exit_code;
}


