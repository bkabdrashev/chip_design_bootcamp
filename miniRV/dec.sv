module dec (
  input logic [REG_END_WORD:0] inst,
  input logic clock,

  output logic [REG_END_ID:0] rd,
  output logic [REG_END_ID:0] rs1,
  output logic [REG_END_ID:0] rs2,

  output logic [REG_END_WORD:0] imm,
  output logic [3:0] alu_op,
  output logic [3:0] mem_wbmask,
  output logic is_mem_sign,
  output logic ebreak,
  output logic [2:0] inst_type
);
/* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
/* verilator lint_on UNUSEDPARAM */

  logic sign; 
  logic sub;

  logic [6:0] opcode;
  logic [2:0] funct3;

  logic [REG_END_WORD:0]   i_imm; 
  logic [REG_END_WORD:0]   u_imm; 
  logic [REG_END_WORD:0]   s_imm; 

  always_comb begin
    opcode = inst[6:0];
    rd     = inst[11:7];
    funct3 = inst[14:12];
    rs1    = inst[19:15];
    rs2    = inst[24:20];

    sign = inst[31];
    sub = inst[30];

    imm = 0;
    ebreak = 0;

    i_imm = { {20{sign}}, inst[31:20] };
    u_imm = { inst[31:12], 12'd0 };
    s_imm = { {20{sign}}, inst[31:25], inst[11:7] };

    inst_type = 0;

    case (opcode)
      OPCODE_CALC_IMM: begin
        inst_type = INST_IMM;
      end
      OPCODE_CALC_REG: begin
        inst_type = INST_REG;
      end
      OPCODE_LOAD: begin
        imm = i_imm;
      end
      OPCODE_STORE: begin
        imm = s_imm;
        inst_type = INST_STORE;
      end
      OPCODE_LUI: begin
        imm = u_imm;
        inst_type = INST_IMM;
      end
      OPCODE_JALR: begin
        imm = i_imm;
        inst_type = INST_JUMP;
      end
      OPCODE_ENV: begin
        ebreak = inst[20] && clock;
      end
      default: ;
    endcase

    case (funct3)
      FUNCT3_BYTE:        begin mem_wbmask = 4'b0001; inst_type = {1'b0,funct3[1:0]}; end
      FUNCT3_HALF:        begin mem_wbmask = 4'b0011; inst_type = {1'b0,funct3[1:0]}; end
      FUNCT3_WORD:        begin mem_wbmask = 4'b1111; inst_type = {1'b0,funct3[1:0]}; end
      FUNCT3_BYTE_UNSIGN: begin mem_wbmask = 4'b0001; inst_type = {1'b0,funct3[1:0]}; end
      FUNCT3_HALF_UNSIGN: begin mem_wbmask = 4'b0011; inst_type = {1'b0,funct3[1:0]}; end
      default:            begin mem_wbmask = 4'b0000; inst_type = 0;                  end
    endcase

    is_mem_sign = funct3[2];
    alu_op = {sub,funct3};
  end

endmodule;

