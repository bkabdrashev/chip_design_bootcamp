module lsu (
  input logic         clock,
  input logic         reset,

  input               is_lsu,
  input  logic [31:0] rdata,
  input  logic [31:0] wdata,
  input  logic [1:0]  addr,
  input  logic [1:0]  data_size,
  input  logic        is_mem_sign,

  input  logic        respValid,
  output logic        reqValid,
  output logic        is_busy,
  output logic [31:0] lsu_wdata,
  output logic [31:0] lsu_rdata,
  output logic [3:0]  lsu_wmask);

  localparam LSU_BYTE = 2'b00;
  localparam LSU_HALF = 2'b01;
  localparam LSU_WORD = 2'b10;
  localparam LSU_NONE = 2'b11;

  logic [31:0] align_rdata;
  logic [1:0]  addr_offset;
  logic        mem_byte_sign   = align_rdata[ 7] & is_mem_sign;
  logic        mem_half_sign   = align_rdata[15] & is_mem_sign;
  logic [23:0] mem_byte_extend = {24{mem_byte_sign}};
  logic [15:0] mem_half_extend = {16{mem_half_sign}};

  assign addr_offset = addr[1:0];
  always_comb begin
    case (addr_offset)
      2'b00: lsu_wdata =  wdata[31:0];
      2'b01: lsu_wdata = {wdata[23:0], wdata[31:24]};
      2'b10: lsu_wdata = {wdata[15:0], wdata[31:16]};
      2'b11: lsu_wdata = {wdata[ 7:0], wdata[31: 8]};
    endcase

    case (data_size)
      LSU_BYTE: begin
        case (addr_offset)
          2'b00: lsu_wmask = 4'b0001;
          2'b01: lsu_wmask = 4'b0010;
          2'b10: lsu_wmask = 4'b0100;
          2'b11: lsu_wmask = 4'b1000;
        endcase
      end
      LSU_HALF: begin
        case (addr_offset)
          2'b00: lsu_wmask = 4'b0011;
          2'b01: lsu_wmask = 4'b0110;
          2'b10: lsu_wmask = 4'b1100;
          2'b11: lsu_wmask = 4'b1000;
        endcase
      end
      LSU_WORD: begin
        case (addr_offset)
          2'b00: lsu_wmask = 4'b1111;
          2'b01: lsu_wmask = 4'b1110;
          2'b10: lsu_wmask = 4'b1100;
          2'b11: lsu_wmask = 4'b1000;
        endcase
      end
      LSU_NONE: begin
        lsu_wmask = 4'b1111;
      end
    endcase
  end

  always_comb begin
    case (addr_offset)
      2'b00: align_rdata =  rdata[31:0];
      2'b01: align_rdata = {rdata[ 7:0], rdata[31: 8]};
      2'b10: align_rdata = {rdata[15:0], rdata[31:16]};
      2'b11: align_rdata = {rdata[23:0], rdata[31:24]};
    endcase
  end

  assign mem_byte_sign   = align_rdata[ 7] & is_mem_sign;
  assign mem_half_sign   = align_rdata[15] & is_mem_sign;
  assign mem_byte_extend = {24{mem_byte_sign}};
  assign mem_half_extend = {16{mem_half_sign}};
  always_comb begin
    case (data_size)
      LSU_BYTE: lsu_rdata = {mem_byte_extend, align_rdata[ 7:0]};
      LSU_HALF: lsu_rdata = {mem_half_extend, align_rdata[15:0]};
      LSU_WORD: lsu_rdata = align_rdata[31:0];
      LSU_NONE: lsu_rdata = align_rdata[31:0];
    endcase
  end

  typedef enum logic {
    IDLE, WAIT
  } lsu_state;

  lsu_state next_state;
  lsu_state curr_state;

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      curr_state <= IDLE;
    end else begin
      curr_state <= next_state;
    end
  end

  always_comb begin
    reqValid = 1'b0;
    case (curr_state)
      IDLE: begin
        if (is_lsu) begin
          next_state    = WAIT;
          reqValid  = 1'b1;
        end
        else next_state = IDLE;
      end
      WAIT: begin
        if (respValid) next_state = IDLE;
        else           next_state = WAIT;
      end
    endcase
  end

  assign is_busy = next_state != IDLE;
endmodule
