module ram (
  input  logic                  clock,
  input  logic                  reset,
  input  logic                  wen,
  input  logic [REG_END_WORD:0] wdata,
  input  logic [3:0]            wbmask,
  input  logic [REG_END_WORD:0] addr,    

  output logic [REG_END_WORD:0] rdata
);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */

  export "DPI-C" function sv_mem_read;
  function int unsigned sv_mem_read(input int unsigned mem_addr);
    int unsigned result;
    mem_read(mem_addr, result);
    return result;
  endfunction

  export "DPI-C" function sv_mem_ptr;
  function longint unsigned sv_mem_ptr();
    longint unsigned mem_ptr_out;
    mem_ptr(mem_ptr_out);
    return mem_ptr_out;
  endfunction

  export "DPI-C" function sv_vga_ptr;
  function longint unsigned sv_vga_ptr();
    longint unsigned vga_ptr_out;
    vga_ptr(vga_ptr_out);
    return vga_ptr_out;
  endfunction

  export "DPI-C" function sv_uart_ptr;
  function longint unsigned sv_uart_ptr();
    longint unsigned uart_ptr_out;
    uart_ptr(uart_ptr_out);
    return uart_ptr_out;
  endfunction

  export "DPI-C" function sv_time_ptr;
  function longint unsigned sv_time_ptr();
    longint unsigned time_ptr_out;
    time_ptr(time_ptr_out);
    return time_ptr_out;
  endfunction

  export "DPI-C" function sv_mem_write;
  function void sv_mem_write(input int unsigned mem_addr, input int unsigned mem_wdata, input byte mem_wbmask);
    mem_write(mem_addr, mem_wdata, mem_wbmask);
  endfunction

  import "DPI-C" context task mem_write(input int unsigned address, input int unsigned write, input byte wbmask);
  import "DPI-C" context task mem_read(input int unsigned address, output int unsigned read);
  import "DPI-C" context task mem_reset();
  import "DPI-C" context task mem_ptr(output longint unsigned mem_ptr_out);
  import "DPI-C" context task vga_ptr(output longint unsigned vga_ptr_out);
  import "DPI-C" context task uart_ptr(output longint unsigned uart_ptr_out);
  import "DPI-C" context task time_ptr(output longint unsigned time_ptr_out);

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      mem_reset();
    end else begin
      if (wen) mem_write(addr, wdata, {4'b0, wbmask});
    end
  end

  always_comb begin
    mem_read(addr, rdata);
  end

endmodule


