module ifu (
  input logic  clock,
  input logic  reset,

  input  logic is_fetch_inst,
  input  logic respValid,
  output logic reqValid,
  output logic is_inst_ready,
  output logic is_busy);

  typedef enum logic {
    IDLE, WAIT
  } ifu_state;

  ifu_state next_state;
  ifu_state curr_state;

  always_ff @(posedge clock or posedge reset) begin
    if (reset) begin
      curr_state <= IDLE;
    end else begin
      curr_state <= next_state;
    end
  end

  always_comb begin
    reqValid      = 1'b0;
    is_inst_ready = 1'b0;
    case (curr_state)
      IDLE: begin
        if (is_fetch_inst) begin
          reqValid    = 1'b1;
          next_state  = WAIT;
        end
        else next_state = IDLE;
      end
      WAIT: begin
        if (respValid) begin
          next_state    = IDLE;
          is_inst_ready = 1'b1;
        end
        else next_state = WAIT;
      end
    endcase
  end

  assign is_busy = curr_state != IDLE && !respValid;

endmodule

