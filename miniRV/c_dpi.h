#include "mem_map.h"

extern uint8_t  memory[MEM_SIZE];
extern uint8_t  uart_status;
extern uint64_t time_uptime;

extern uint64_t hash_uint64_t(uint64_t x);
extern "C" uint32_t mem_read(uint32_t address);
extern "C" void mem_write(uint32_t address, uint32_t write_data, uint8_t wstrb);
extern "C" bool mem_request();
extern "C" void mem_reset();
