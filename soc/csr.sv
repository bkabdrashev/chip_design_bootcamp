module csr (
  input  logic               clock,
  input  logic               reset,

  input  logic               is_ebreak,
  input  logic               is_instret,
  input  logic               is_ifu_wait,
  input  logic               is_lsu_wait,
  input  logic               is_load_seen,
  input  logic               is_store_seen,
  input  logic               is_calc_seen,
  input  logic               is_jump_seen,
  input  logic               is_branch_seen,
  input  logic               is_branch_taken,

/* verilator lint_off UNUSEDSIGNAL */
  input  logic               wen,
/* verilator lint_on UNUSEDSIGNAL */

  input  logic [11:0]        addr,
/* verilator lint_off UNUSEDSIGNAL */
  input  logic [REG_W_END:0] wdata,
/* verilator lint_on UNUSEDSIGNAL */
  output logic [REG_W_END:0] rdata);

/* verilator lint_off UNUSEDPARAM */
`include "reg_defines.vh"
/* verilator lint_on UNUSEDPARAM */

  localparam MCYCLE        = 12'hB00;
  localparam MINSTRET      = 12'hB02;
  localparam MIFU_WAIT     = 12'hB03;
  localparam MLSU_WAIT     = 12'hB04;
  localparam MLOAD_SEEN    = 12'hB05;
  localparam MSTORE_SEEN   = 12'hB06;
  localparam MCALC_SEEN    = 12'hB07;
  localparam MBRANCH_SEEN  = 12'hB08;
  localparam MBRANCH_TAKEN = 12'hB09;
  localparam MJUMP_SEEN    = 12'hB0A;

  localparam MCYCLEH        = 12'hB80;
  localparam MINSTRETH      = 12'hB82;
  localparam MIFU_WAITH     = 12'hB83;
  localparam MLSU_WAITH     = 12'hB84;
  localparam MLOAD_SEENH    = 12'hB85;
  localparam MSTORE_SEENH   = 12'hB86;
  localparam MCALC_SEENH    = 12'hB87;
  localparam MBRANCH_SEENH  = 12'hB88;
  localparam MBRANCH_TAKENH = 12'hB89;
  localparam MJUMP_SEENH    = 12'hB8A;

  localparam MVENDORID     = 12'hf11;
  localparam MARCHID       = 12'hf12;
  localparam MISA          = 12'h301;

  // extensions:                 MXL    ZY XWVUTSRQ PONMLKJI HGFEDCBA
  localparam MISA_VAL      = 32'b01_000000_00000000_00000000_00010000;
  localparam MVENDORID_VAL = "akeb";
  localparam MARCHID_VAL   = 32'h05318008; 

  localparam MHPMCOUNTER_N = 8; 

  logic [63:0] mcycle,   mcycle_inc;
  logic [63:0] minstret, minstret_inc;
  logic [63:0] mhpmcounter     [0:MHPMCOUNTER_N-1];
  logic [63:0] mhpmcounter_inc [0:MHPMCOUNTER_N-1];

  generate
    for (genvar i = 0; i < MHPMCOUNTER_N; i++) begin : gen_mhpcounter_ff
      always_ff @(posedge clock or posedge reset) begin
        if (reset) begin
          mhpmcounter[i] <= 64'h0;
        end
        else begin
          mhpmcounter[i] <= mhpmcounter_inc[i];
        end
      end
    end
  endgenerate

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      mcycle   <= 64'h0;
      minstret <= 64'h0;
    end
    else begin
      mcycle   <= mcycle_inc;
      minstret <= minstret_inc;
    end
  end

  logic [2:0] mhpmcounter_idx;
  assign mhpmcounter_idx = addr[4:2];
  always_comb begin
    case (addr) 
      MISA:      rdata = MISA_VAL;
      MARCHID:   rdata = MARCHID_VAL;
      MVENDORID: rdata = MVENDORID_VAL;
      MCYCLE:    rdata = mcycle[31: 0];
      MCYCLEH:   rdata = mcycle[63:32];
      MINSTRET:  rdata = minstret[31: 0];
      MINSTRETH: rdata = minstret[63:32];

      MIFU_WAIT,  MLSU_WAIT,    MLOAD_SEEN,    MSTORE_SEEN,
      MCALC_SEEN, MBRANCH_SEEN, MBRANCH_TAKEN, MJUMP_SEEN:
        rdata = mhpmcounter[mhpmcounter_idx][31: 0];

      MIFU_WAITH,  MLSU_WAITH,    MLOAD_SEENH,    MSTORE_SEENH,
      MCALC_SEENH, MBRANCH_SEENH, MBRANCH_TAKENH, MJUMP_SEENH:
        rdata = mhpmcounter[mhpmcounter_idx][63:32];

      default: rdata = 32'h0;
    endcase
  end

  always_comb begin

    mcycle_inc = mcycle;
    if (~is_ebreak)   mcycle_inc = mcycle + 1;

    minstret_inc = minstret;
    if (is_instret)  minstret_inc = minstret + 1;

    mhpmcounter_inc[0] = mhpmcounter[0];
    mhpmcounter_inc[1] = mhpmcounter[1];
    mhpmcounter_inc[2] = mhpmcounter[2];
    mhpmcounter_inc[3] = mhpmcounter[3];
    mhpmcounter_inc[4] = mhpmcounter[4];
    mhpmcounter_inc[5] = mhpmcounter[5];
    mhpmcounter_inc[6] = mhpmcounter[6];
    mhpmcounter_inc[7] = mhpmcounter[7];

    if (is_ifu_wait)     mhpmcounter_inc[0] = mhpmcounter[0] + 1;
    if (is_lsu_wait)     mhpmcounter_inc[1] = mhpmcounter[1] + 1;
    if (is_load_seen)    mhpmcounter_inc[2] = mhpmcounter[2] + 1;
    if (is_store_seen)   mhpmcounter_inc[3] = mhpmcounter[3] + 1;
    if (is_calc_seen)    mhpmcounter_inc[4] = mhpmcounter[4] + 1;
    if (is_jump_seen)    mhpmcounter_inc[5] = mhpmcounter[5] + 1;
    if (is_branch_seen)  mhpmcounter_inc[6] = mhpmcounter[6] + 1;
    if (is_branch_taken) mhpmcounter_inc[7] = mhpmcounter[7] + 1;
  end

endmodule

