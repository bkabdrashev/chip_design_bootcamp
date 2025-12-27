module ram (
  input  logic                  clock,
  input  logic                  reset,
  input  logic                  reqValid,

  input  logic                  wen,
  input  logic [REG_END_WORD:0] wdata,
  input  logic [3:0]            wbmask,
  input  logic [REG_END_WORD:0] addr,    

  output logic                  respValid,
  output logic [REG_END_WORD:0] rdata
);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */

  import "DPI-C" context task mem_write(input int unsigned address, input int unsigned write, input byte wbmask);
  import "DPI-C" context function int unsigned mem_read(input int unsigned address);
  import "DPI-C" context function bit mem_request();
  import "DPI-C" context task mem_reset();

  logic                  busy;
  // logic [1:0]            counter;
  logic [REG_END_WORD:0] addr_q;
  logic                  wen_q;
  logic [REG_END_WORD:0] wdata_q;
  logic [3:0] wbmask_q;

  always_ff @(posedge clock, posedge reset) begin
    if (reset) begin
      mem_reset();
      busy      <= 1'b0;
      respValid <= 1'b0;
      // counter   <= '0;
      rdata     <= '0;
      addr_q    <= '0;
      wen_q     <= '0;
      wdata_q   <= '0;
      wbmask_q  <= '0;
    end
    else begin
      respValid <= 1'b0;

      if (!busy) begin
        if (reqValid) begin
          busy     <= 1'b1;
          // counter  <= 2'd0;
          addr_q   <= addr;
          wen_q    <= wen;
          wdata_q  <= wdata;
          wbmask_q <= wbmask;
        end
      end
      else begin
        if (mem_request()) begin
          if (wen_q) begin
            rdata <= 32'b0;
            mem_write(addr_q, wdata_q, {4'b0, wbmask_q});
          end
          else begin
            rdata <= mem_read(addr_q);
          end
          respValid <= 1'b1;
          busy      <= 1'b0;
          // $display("rdata: %d, wen: %d", rdata, wen);
        end
      end
    end
  end

endmodule


