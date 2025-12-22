module rf (
  input logic       clk,
  input logic       reset,
  input logic       wen,
  input logic [4:0]  rd,
  input logic [4:0] rs1,
  input logic [4:0]  rs2,
  input logic [31:0] wdata,
  output logic [31:0] regs_out [0:15],
  output logic [31:0] rdata1,
  output logic [31:0] rdata2
);

  logic [31:0] regs [0:15];
  logic [3:0] trunc_rd;
  logic [3:0] trunc_rs1;
  logic [3:0] trunc_rs2;
  integer i;
  integer j;

  always_ff @(posedge clk or posedge reset) begin
    if (reset) begin
      for (i = 0; i < 16; i++) begin
        regs[i] <= 32'h0;
      end
    end else if (wen && (rd != 0) && (rd < 16)) begin
      regs[trunc_rd] <= wdata;
    end
  end

  always_comb begin
    trunc_rd = rd[3:0];
    trunc_rs1 = rs1[3:0];
    trunc_rs2 = rs2[3:0];
    rdata1 = (rs1 == 0 || rs1 >= 16) ? 32'h0 : regs[trunc_rs1];
    rdata2 = (rs2 == 0 || rs2 >= 16) ? 32'h0 : regs[trunc_rs2];
    for (j = 0; j < 16; j++) begin
      regs_out[j] = regs[j];
    end
  end

endmodule;


