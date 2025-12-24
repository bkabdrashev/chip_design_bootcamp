module miniRV (
  input logic reg_reset,
  input logic mem_reset,
  input logic rom_wen,
  input logic [31:0] rom_wdata,
  input logic [31:0] rom_addr,
  input logic clk,
  output logic [31:0] regs_out [0:15],
  output logic [31:0] pc,
  output logic ebreak
);

  logic [31:0] inst;
  logic [4:0] rd;
  logic [4:0] rs1;
  logic [4:0] rs2;
  logic [31:0] imm;
  logic        reg_wen;
  logic [6:0]  opcode;
  logic [2:0]  funct3;
  logic [6:0]  funct7;

  logic is_pc_jump;
  logic [31:0] pc_addr;

  logic [31:0] rdata1;
  logic [31:0] rdata2;
  logic [31:0] alu_res;

  logic [31:0] reg_wdata;

  logic        ram_wen;
 /* verilator lint_off UNOPTFLAT */
  logic [31:0] ram_addr;
  logic [31:0] ram_wdata;
  logic [31:0] ram_rdata;
  logic [3:0]  ram_wstrb;

  pc u_pc(
    .clk(clk),
    .reset(reg_reset),
    .block_increment(rom_wen),
    .in_addr(pc_addr),
    .is_addr(is_pc_jump),
    .out_addr(pc)
  );

  ram u_ram(
    .clk(clk),
    .reset(mem_reset),
    .wen(ram_wen),
    .wdata(ram_wdata),
    .wstrb(ram_wstrb),
    .addr(ram_addr),

    .read_data(ram_rdata));

  rom u_rom(
    .addr(pc),

    .read_data(inst));

  dec u_dec(
    .inst(inst),

    .rd(rd),
    .rs1(rs1),
    .rs2(rs2),
    .imm(imm),
    .wen(reg_wen),
    .opcode(opcode),
    .funct3(funct3),
    .funct7(funct7)
  );

  alu u_alu(
    .opcode(opcode),
    .rdata1(rdata1),
    .rdata2(rdata2),
    .imm(imm),
    .funct3(funct3),
    .funct7(funct7),

    .rout(alu_res)
  );

  rf u_rf(
    .clk(clk),
    .reset(reg_reset),
    .wen(reg_wen),
    .rd(rd),
    .wdata(reg_wdata),
    .rs1(rs1),
    .rs2(rs2),

    .rdata1(rdata1),
    .rdata2(rdata2),
    .regs_out(regs_out)
  );

  always_comb begin
    ram_wen = 0;
    ram_addr = 0;
    ram_wdata = 0;
    ram_wstrb = 4'b0000;

    reg_wdata = 0;
    pc_addr = 0;
    is_pc_jump = 0;

    ebreak = 0;
    if (rom_wen) begin
      ram_wen = 1;
      ram_addr = rom_addr;
      ram_wdata = rom_wdata;
      ram_wstrb = 4'b1111;
    end else begin
      if (opcode == 7'b0010011) begin // ADDI, SLLI, SRLI, SRAI
        reg_wdata = alu_res;
      end else if (opcode == 7'b1100111) begin
        // JALR
        reg_wdata = pc+4;
        pc_addr = (rdata1 + imm) & ~3;
        is_pc_jump = 1;
      end else if (opcode == 7'b0110011) begin
        // ADD
        reg_wdata = alu_res;
      end else if (opcode == 7'b0110111) begin
        // LUI
        reg_wdata = imm;
      end else if (opcode == 7'b0000011 && funct3 == 3'b010) begin
        // LW
        ram_addr = rdata1 + imm;
        reg_wdata = ram_rdata;
      end else if (opcode == 7'b0000011 && funct3 == 3'b100) begin
        // LBU
        ram_addr = rdata1 + imm;

        reg_wdata = ram_rdata & 32'hff;
      end else if (opcode == 7'b0100011 && funct3 == 3'b010) begin
        // SW
        ram_wen = 1;
        ram_addr = rdata1 + imm;
        ram_wdata = rdata2;
        ram_wstrb = 4'b1111;

      end else if (opcode == 7'b0100011 && funct3 == 3'b000) begin
        // SB
        ram_wen = 1;
        ram_addr = rdata1 + imm;
        ram_wdata = rdata2 & 32'hff;
        ram_wstrb = 4'b0001;
      end else if (opcode == 7'b1110011 && imm == 1) begin
        // EBREAK
        ebreak = 1;
        // $finish;
      end else begin
        // $strobe ("DUT WARNING: not implemented %h", clk);
        // NOT IMPLEMENTED
      end
    end
  end

endmodule;


