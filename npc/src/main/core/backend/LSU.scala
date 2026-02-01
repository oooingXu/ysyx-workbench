package npc

import chisel3._
import chisel3.util._

class ysyx_23060336_LSU extends Module{
  val io = IO(new Bundle{
    val exu_lsu_data = Flipped(Decoupled(new EXU_LSU_DATA()))
    val lsu_wbu_data = Decoupled(new LSU_WBU_DATA())
    val lsu_idu_raw  = new LSU_IDU_RAW()
    val axi          = new ysyx_23060336_AXI4Master()
  })

  val prepare  = Wire(Bool())
  val MemtoReg = Wire(Bool())
  val MemWr    = Wire(Bool())

  val src2    = Wire(UInt(Base.dataWidth.W))
  val DataOut = Wire(UInt(Base.dataWidth.W))
  val rdata   = Wire(UInt(Base.dataWidth.W))
  val result  = Wire(UInt(Base.dataWidth.W))

  val size    = Wire(UInt(2.W)) // 3(sizeWidth) - 1

  val wdata_b = Wire(UInt(Base.dataWidth.W))
  val wdata_h = Wire(UInt(Base.dataWidth.W))

  val wstrb   = Wire(UInt(Base.wstrbWidth.W))
  val wstrb_b = Wire(UInt(Base.wstrbWidth.W))
  val wstrb_h = Wire(UInt(Base.wstrbWidth.W))
  
  val rdata_b = Wire(UInt(Base.dataWidth.W))
  val rdata_h = Wire(UInt(Base.dataWidth.W))

  val func3 = Wire(UInt(3.W))

  // state machine
  val s_idle :: s_wait_rslave :: s_wait_wslave :: s_wait_prepare :: s_wait_sign :: Nil = Enum(5)
  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(List(
    s_idle         -> Mux(io.exu_lsu_data.valid, s_wait_sign, s_idle),
    s_wait_sign    -> Mux(MemtoReg, s_wait_rslave, Mux(MemWr, s_wait_wslave, Mux(io.exu_lsu_data.valid, s_wait_sign, s_idle))),
    s_wait_rslave  -> Mux(io.axi.arready, Mux(io.axi.rvalid && io.axi.rlast, Mux(io.exu_lsu_data.valid, s_wait_sign, s_idle), s_wait_prepare), s_wait_rslave),
    s_wait_wslave  -> Mux(io.axi.wready, s_wait_prepare, s_wait_wslave),
    s_wait_prepare -> Mux(prepare, Mux(io.exu_lsu_data.valid, s_wait_sign, s_idle), s_wait_prepare)
  ))

  io.exu_lsu_data.ready := state === s_idle || (state === s_wait_sign && !MemtoReg && !MemWr) || (state === s_wait_rslave && io.axi.arready && io.axi.rvalid && io.axi.rlast) || (state === s_wait_prepare && prepare && io.exu_lsu_data.valid)
  io.lsu_wbu_data.valid := (state === s_wait_sign && !MemtoReg && !MemWr) || (state === s_wait_prepare && prepare) || (state === s_wait_rslave && io.axi.arready && io.axi.rvalid && io.axi.rlast)

  // signal
  result   := io.exu_lsu_data.bits.result
  src2     := io.exu_lsu_data.bits.idu_lsu_data.src2
  MemtoReg := io.exu_lsu_data.bits.idu_lsu_data.MemtoReg
  MemWr    := io.exu_lsu_data.bits.idu_lsu_data.MemWr

  wstrb := MuxLookup(func3, "hf".U(4.W))(
    Seq(
      "h0".U -> "h1".U,
      "h1".U -> "h3".U,
      "h2".U -> "hf".U
      )
  )

  size  := func3(1, 0)
  func3 := io.exu_lsu_data.bits.idu_lsu_data.func3

  // rdata
  rdata_h := Mux(result(1,0) === 2.U, Cat(0.U(16.W), io.axi.rdata(31, 16)), Cat(0.U(16.W), io.axi.rdata(15, 0)))

  rdata_b := Mux(result(1,0) === 3.U, Cat(0.U(24.W), io.axi.rdata(31, 24)),
             Mux(result(1,0) === 2.U, Cat(0.U(24.W), io.axi.rdata(23, 16)),
             Mux(result(1,0) === 1.U, Cat(0.U(24.W), io.axi.rdata(15,  8)), Cat(0.U(24.W), io.axi.rdata(7, 0)))))

  // wdata
  wdata_h := Mux(result(1,0) === 2.U, Cat(src2(15, 0), 0.U(16.W)), Cat(0.U(16.W), src2(15,0)))

  wdata_b := Mux(result(1,0) === "b11".U, Cat(src2(7,0), 0.U(24.W)),
             Mux(result(1,0) === "b10".U, Cat(0.U(8.W),  src2(7,0), 0.U(16.W)),
             Mux(result(1,0) === "b01".U, Cat(0.U(16.W), src2(7,0), 0.U(8.W)), Cat(0.U(24.W), src2(7,0)))))


  // wstrb
  wstrb_h := Mux(result(1,0) === 2.U, wstrb << 2, 
             Mux(result(1,0) === 0.U, wstrb, 0.U))

  wstrb_b := Mux(result(1,0) === 3.U, wstrb << 3, 
             Mux(result(1,0) === 2.U, wstrb << 2, 
             Mux(result(1,0) === 1.U, wstrb << 1, wstrb)))

  prepare := (MemtoReg && io.axi.rvalid && io.axi.rlast) || (MemWr && io.axi.bvalid) 
  DataOut := Mux(size === 0.U, rdata_b, Mux(size === 1.U, rdata_h, io.axi.rdata))

  // lsu <> wbu
  io.lsu_wbu_data.bits.idu_wbu_data <> io.exu_lsu_data.bits.idu_lsu_data.idu_wbu_data
  io.lsu_wbu_data.bits.exu_wbu_data <> io.exu_lsu_data.bits.exu_wbu_data

  io.lsu_wbu_data.bits.csrdata := result
  io.lsu_wbu_data.bits.regdata := Mux(MemtoReg, rdata, result)

  rdata := Mux(func3 === "b000".U, Cat(Fill(24, DataOut(7)), DataOut(7, 0)),
           Mux(func3 === "b001".U, Cat(Fill(24, DataOut(15)), DataOut(15, 0)), DataOut))     

  // sram_read
  io.lsu_wbu_data.bits.wbu_sram_data.wstrb   := wstrb
  io.lsu_wbu_data.bits.wbu_sram_data.araddr  := io.axi.araddr
  io.lsu_wbu_data.bits.wbu_sram_data.awaddr  := io.axi.awaddr
  io.lsu_wbu_data.bits.wbu_sram_data.wdata   := src2
  io.lsu_wbu_data.bits.wbu_sram_data.arsize  := io.axi.arsize
  io.lsu_wbu_data.bits.wbu_sram_data.arvalid := MemtoReg
  io.lsu_wbu_data.bits.wbu_sram_data.awvalid := MemWr

  // AXI4
  io.axi.awvalid := Mux(reset.asBool, false.B, MemWr && (state === s_wait_wslave))
  io.axi.awaddr  := result
  io.axi.awid    := "h2".U
  io.axi.awlen   := "h0".U
  io.axi.awsize  := Cat(0.U(1.W), size)
  io.axi.awburst := "h1".U
  io.axi.wvalid  := Mux(reset.asBool, false.B, MemWr && (state === s_wait_wslave))
  io.axi.wdata   := Mux(size === 0.U, wdata_b, Mux(size === 1.U, wdata_h, src2))
  io.axi.wlast   := MemWr && (state === s_wait_wslave)
  io.axi.bready  := true.B
  io.axi.arvalid := Mux(reset.asBool, false.B, (state === s_wait_rslave) && MemtoReg)
  io.axi.araddr  := result 
  io.axi.arid    := "h2".U
  io.axi.arlen   := "h0".U
  io.axi.arsize  := Cat(0.U(1.W), size)
  io.axi.arburst := "h1".U
  io.axi.rready  := state === s_idle || state === s_wait_rslave || state === s_wait_prepare
  io.axi.wstrb   := Mux(size === 0.U, wstrb_b, Mux(size === 1.U, wstrb_h, wstrb))

  // lsu <> idu 
  io.lsu_idu_raw.lsu_valid := (state === s_wait_sign && !MemtoReg && !MemWr) || io.axi.rlast
  io.lsu_idu_raw.lsu_regdata  := io.lsu_wbu_data.bits.regdata
  io.lsu_idu_raw.lsu_rd       := io.exu_lsu_data.bits.idu_lsu_data.idu_wbu_data.rd
  io.lsu_idu_raw.lsu_rden     := io.exu_lsu_data.bits.idu_lsu_data.idu_wbu_data.rden
  io.lsu_idu_raw.lsu_MemtoReg := MemtoReg

  // useCounter
  if(Config.useDebug) {
    val lsu_counter = Module(new LSU_COUNTER())
    lsu_counter.io.clock := clock
    lsu_counter.io.state := state

    val pc_debug = Wire(UInt(Base.addrWidth.W))
    val dnpc_debug = Wire(UInt(Base.addrWidth.W))

    pc_debug   := Cat(io.lsu_wbu_data.bits.exu_wbu_data.pc, 0.U(2.W))
    dnpc_debug := Cat(io.lsu_wbu_data.bits.exu_wbu_data.dnpc, 0.U(2.W))

    dontTouch(pc_debug)
    dontTouch(dnpc_debug)
  }
}

