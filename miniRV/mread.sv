module mread (
  input logic [REG_END_WORD:0] addr,    

  output logic [REG_END_WORD:0] rdata
);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */

  import "DPI-C" context task mem_read(input int unsigned address, output int unsigned read);
  always_comb begin
    mem_read(addr, rdata);
  end
endmodule


