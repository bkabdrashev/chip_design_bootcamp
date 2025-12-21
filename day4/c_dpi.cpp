#include "svdpi.h"
#include <stdio.h>
extern "C" void print_ram_input(int wen, int wdata, char wstrb, int addr) {
  printf("%i, %i, %i, %i\n", wen, wdata, wstrb, addr);
}
