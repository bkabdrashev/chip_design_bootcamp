module ram64k #(
  parameter int unsigned BYTES = 65536
) (
  input  logic        clk,
  input  logic        wen,
  input  logic [31:0] wdata,
  input  logic [3:0]  wstrb,
  input  logic [31:0] addr,      // byte address
  output logic [31:0] read_data
);

  logic [7:0] mem [0:BYTES-1];

  always_ff @(posedge clk) begin
    if (wen && addr < BYTES) begin
      if (wstrb[0]) mem[addr + 0] <= wdata[7:0];
      if (wstrb[1]) mem[addr + 1] <= wdata[15:8];
      if (wstrb[2]) mem[addr + 2] <= wdata[23:16];
      if (wstrb[3]) mem[addr + 3] <= wdata[31:24];
    end
  end

  always_comb begin
    if (addr < BYTES) begin
      read_data = { mem[addr+3], mem[addr+2], mem[addr+1], mem[addr+0] };
    end else begin
      read_data = 32'h0000_0000;
    end
  end

endmodule

