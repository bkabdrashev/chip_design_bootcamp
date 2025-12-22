#include "svdpi.h"
#include <stdio.h>
extern "C" void get_mem_write(unsigned int address, unsigned int write) {
  if (address >= 0x20000000) {
    printf("mem[%u] write: %u\n", address, write);
  }
}

extern "C" void get_mem_read(unsigned int address, unsigned int read) {
  if (address >= 0x20000000) {
    printf("mem[%u] read: %u\n", address, read);
  }
}
