module ram64k #(
  parameter int unsigned BYTES = 16000000,
  parameter string HEXFILE     = ""
) (
  input  logic        clk,
  input  logic        reset,
  input  logic        wen,
  input  logic [31:0] wdata,
  input  logic [3:0]  wstrb,
  input  logic [31:0] addr,      // byte address
  output logic [31:0] read_data
);
  initial begin
    if (HEXFILE != "") $readmemh(HEXFILE, mem);
  end
  logic [7:0] mem [0:BYTES-1];

  export "DPI-C" function mem_get;
  function byte mem_get(input int unsigned get_addr);
    return mem[get_addr];
  endfunction

  // import "DPI-C" context task get_mem_write(
  //   input int unsigned address,
  //   input int unsigned write,
  // );

  // import "DPI-C" context task get_mem_read(
  //   input int unsigned address,
  //   input int unsigned read,
  // );

  always_ff @(posedge clk or posedge reset) begin
    if (reset) begin
      for (int i = 0; i < BYTES; i++) mem[i] <= 0;
    end else if (wen && addr < BYTES) begin
      // get_mem_write(addr, wdata);
      if (wstrb[0]) mem[addr + 0] <= wdata[7:0];
      if (wstrb[1]) mem[addr + 1] <= wdata[15:8];
      if (wstrb[2]) mem[addr + 2] <= wdata[23:16];
      if (wstrb[3]) mem[addr + 3] <= wdata[31:24];
    end
  end

  always_comb begin
    if (addr < BYTES) begin
      read_data = { mem[addr+3], mem[addr+2], mem[addr+1], mem[addr+0] };
      // get_mem_read(addr, read_data);
    end else begin
      read_data = 32'h0000_0000;
    end
  end

endmodule

