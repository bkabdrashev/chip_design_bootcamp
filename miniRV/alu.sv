module alu (
  input logic [6:0] opcode,
  input logic [31:0] rdata1,
  input logic [31:0] rdata2,
  input logic [31:0] imm,
  input logic [2:0] funct3,
  input logic [6:0] funct7,
  output logic [31:0] rout
);

  always_comb begin
    if (opcode == 7'b0010011 && funct3 == 3'b000) begin // ADDI
      rout = rdata1 + imm;
    end else if (opcode == 7'b0010011 && funct3 == 3'b001) begin // SLLI
      rout = rdata1 << imm;
    end else if (opcode == 7'b0010011 && funct3 == 3'b101 && funct7 == 7'b0000000) begin // SRLI
      rout = rdata1 >> imm;
    end else if (opcode == 7'b0010011 && funct3 == 3'b101 && funct7 == 7'b0100000) begin // SRAI
      rout = $signed(rdata1) >>> imm;
    end else if (opcode == 7'b0110011) begin // ADD
      rout = rdata1 + rdata2;
    end else begin 
      rout = 32'd0;
    end
  end

endmodule;




