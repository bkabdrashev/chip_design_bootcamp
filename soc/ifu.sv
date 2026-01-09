module ifu (
  input  logic  clock,
  input  logic  reset,
// IO messages
  input  logic        io_respValid,
  input  logic [31:0] io_rdata,
  output logic [31:0] io_addr,
  output logic        io_reqValid,
// EXU/Pipeline messages
  input  logic [31:0] pc,
  input  logic        reqValid,
  output logic        respValid,
  output logic [31:0] inst);

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
    io_reqValid = 1'b0;
    respValid   = 1'b0;
    inst        = 32'b0;
    next_state  = curr_state;
    case (curr_state)
      IDLE: begin
        if (reqValid) begin
          io_reqValid = 1'b1;
          io_addr     = pc;
          next_state  = WAIT;
        end
        else begin
          next_state = IDLE;
        end
      end
      WAIT: begin
        if (io_respValid) begin
          inst          = io_rdata;
          respValid     = 1'b1;
          if (reqValid) begin
            next_state = WAIT;
            io_reqValid = 1'b1;
          end
          next_state    = IDLE;
        end
        else begin
          next_state = WAIT;
        end
      end
    endcase
  end

endmodule

