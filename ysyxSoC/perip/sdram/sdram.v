module sdram(
  input        clk,
  input        cke,
  input        cs,
  input        ras,
  input        cas,
  input        we,
  input [12:0] a,
  input [ 1:0] ba,
  input [ 1:0] dqm,
  inout [15:0] dq
);
  reg  [3:0]  word_remain_1;
  wire [15:0] _di_buf_din;
  wire [12:0] _rowReg_ext_R0_data;
  wire [15:0] _mem_ext_R0_data;
  wire [3:0]  cmd = {cs, ras, cas, we};
  reg  [12:0] mode;
  reg  [3:0]  word_remain;
  reg  [23:0] raddr_s1;
  reg  [23:0] waddr_s1;
  reg         di_REG;
  reg  [15:0] REG;
  reg  [1:0]  r;
  always @(posedge clk) begin
    automatic logic       _GEN = cke & cmd == 4'h5;
    automatic logic [3:0] _GEN_0;
    automatic logic       _GEN_1 = cke & cmd == 4'h4;
    _GEN_0 = {2'h0, mode[1:0]};
    if (cke & cmd == 4'h0)
      mode <= a;
    word_remain <= _GEN ? 4'h1 << _GEN_0 : (|word_remain) ? word_remain - 4'h1 : 4'h0;
    raddr_s1 <=
      _GEN ? {_rowReg_ext_R0_data, ba, a[8:0]} : raddr_s1 + {23'h0, |word_remain};
    word_remain_1 <=
      _GEN_1 ? 4'h1 << _GEN_0 : (|word_remain_1) ? word_remain_1 - 4'h1 : 4'h0;
    waddr_s1 <=
      _GEN_1
        ? {_rowReg_ext_R0_data, ba, a[8:0]}
        : waddr_s1 + {23'h0, |word_remain_1};
    di_REG <= |word_remain;
    REG <= _di_buf_din;
    r <= ~dqm;
  end // always @(posedge)
  `ifdef ENABLE_INITIAL_REG_
    `ifdef FIRRTL_BEFORE_INITIAL
      `FIRRTL_BEFORE_INITIAL
    `endif // FIRRTL_BEFORE_INITIAL
    initial begin
      automatic logic [31:0] _RANDOM[0:2];
      `ifdef INIT_RANDOM_PROLOG_
        `INIT_RANDOM_PROLOG_
      `endif // INIT_RANDOM_PROLOG_
      `ifdef RANDOMIZE_REG_INIT
        for (logic [1:0] i = 2'h0; i < 2'h3; i += 2'h1) begin
          _RANDOM[i] = `RANDOM;
        end
        mode = _RANDOM[2'h0][12:0];
        word_remain = _RANDOM[2'h0][16:13];
        raddr_s1 = {_RANDOM[2'h0][31:17], _RANDOM[2'h1][8:0]};
        word_remain_1 = _RANDOM[2'h1][12:9];
        waddr_s1 = {_RANDOM[2'h1][31:13], _RANDOM[2'h2][4:0]};
        di_REG = _RANDOM[2'h2][5];
        REG = _RANDOM[2'h2][21:6];
        r = _RANDOM[2'h2][23:22];
      `endif // RANDOMIZE_REG_INIT
    end // initial
    `ifdef FIRRTL_AFTER_INITIAL
      `FIRRTL_AFTER_INITIAL
    `endif // FIRRTL_AFTER_INITIAL
  `endif // ENABLE_INITIAL_REG_
  mem_16777216x16 mem_ext (
    .R0_addr (raddr_s1),
    .R0_en   (1'h1),
    .R0_clk  (clk),
    .R0_data (_mem_ext_R0_data),
    .W0_addr (waddr_s1),
    .W0_en   (|word_remain_1),
    .W0_clk  (clk),
    .W0_data (REG),
    .W0_mask (r)
  );
  rowReg_4x13 rowReg_ext (
    .R0_addr (ba),
    .R0_en   (1'h1),
    .R0_clk  (clk),
    .R0_data (_rowReg_ext_R0_data),
    .W0_addr (ba),
    .W0_en   (cke & cmd == 4'h3),
    .W0_clk  (clk),
    .W0_data (a)
  );
  TriStateInBuf #(
    .width(16)
  ) di_buf (
    .dio    (dq),
    .dout   (_mem_ext_R0_data),
    .out_en (di_REG),
    .din    (_di_buf_din)
  );
  
  //assign dq = 16'bz;

endmodule

module rowReg_4x13(
  input  [1:0]  R0_addr,
  input         R0_en,
                R0_clk,
  output [12:0] R0_data,
  input  [1:0]  W0_addr,
  input         W0_en,
                W0_clk,
  input  [12:0] W0_data
);

  reg [12:0] Memory[0:3];
  always @(posedge W0_clk) begin
    if (W0_en & 1'h1)
      Memory[W0_addr] <= W0_data;
  end // always @(posedge)
  `ifdef ENABLE_INITIAL_MEM_
    reg [31:0] _RANDOM_MEM;
    initial begin
      `INIT_RANDOM_PROLOG_
      `ifdef RANDOMIZE_MEM_INIT
        for (logic [2:0] i = 3'h0; i < 3'h4; i += 3'h1) begin
          _RANDOM_MEM = `RANDOM;
          Memory[i[1:0]] = _RANDOM_MEM[12:0];
        end
      `endif // RANDOMIZE_MEM_INIT
    end // initial
  `endif // ENABLE_INITIAL_MEM_
  assign R0_data = R0_en ? Memory[R0_addr] : 13'bx;
endmodule

module mem_16777216x16(
  input  [23:0] R0_addr,
  input         R0_en,
                R0_clk,
  output [15:0] R0_data,
  input  [23:0] W0_addr,
  input         W0_en,
                W0_clk,
  input  [15:0] W0_data,
  input  [1:0]  W0_mask
);

  reg [15:0] Memory[0:16777215];
  reg        _R0_en_d0;
  reg [23:0] _R0_addr_d0;
  always @(posedge R0_clk) begin
    _R0_en_d0 <= R0_en;
    _R0_addr_d0 <= R0_addr;
  end // always @(posedge)
  always @(posedge W0_clk) begin
    if (W0_en & W0_mask[0])
      Memory[W0_addr][32'h0 +: 8] <= W0_data[7:0];
    if (W0_en & W0_mask[1])
      Memory[W0_addr][32'h8 +: 8] <= W0_data[15:8];
  end // always @(posedge)
  `ifdef ENABLE_INITIAL_MEM_
    `ifdef RANDOMIZE_REG_INIT
      reg [31:0] _RANDOM;
    `endif // RANDOMIZE_REG_INIT
    reg [31:0] _RANDOM_MEM;
    initial begin
      `INIT_RANDOM_PROLOG_
      `ifdef RANDOMIZE_MEM_INIT
        for (logic [24:0] i = 25'h0; i < 25'h1000000; i += 25'h1) begin
          _RANDOM_MEM = `RANDOM;
          Memory[i[23:0]] = _RANDOM_MEM[15:0];
        end
      `endif // RANDOMIZE_MEM_INIT
      `ifdef RANDOMIZE_REG_INIT
        _RANDOM = {`RANDOM};
        _R0_en_d0 = _RANDOM[0];
        _R0_addr_d0 = _RANDOM[24:1];
      `endif // RANDOMIZE_REG_INIT
    end // initial
  `endif // ENABLE_INITIAL_MEM_
  assign R0_data = _R0_en_d0 ? Memory[_R0_addr_d0] : 16'bx;
endmodule

module TriStateInBuf #(
  parameter width = 1
)(
    inout  [width-1:0] dio,
    input  [width-1:0] dout,
    input              out_en,
    output [width-1:0] din
);
  assign din = dio;
  assign dio = out_en ? dout : {width{1'bz}};
endmodule
