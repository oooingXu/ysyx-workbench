package npc

import chisel3._
import chisel3.util._

class ysyx_23060336_IFU extends Module{
  val io = IO(new Bundle{
    val ifu_idu_data = Decoupled(new IFU_IDU_DATA())
    val exu_ifu_raw  = Flipped(new EXU_IFU_RAW())
    val axi          = new ysyx_23060336_AXI4Master()
  })

  val npc = if(Config.useNPCSim) {"h20000000".U(Base.pcWidth.W)} else {"h04000000".U(Base.pcWidth.W)}
  val PC      = RegInit(npc)
  //val finst   = RegInit(0.U(Base.dataWidth.W))
  val araddr  = Wire(UInt(Base.pcWidth.W))
  val preaddr = Wire(UInt(Base.pcWidth.W))

  //val s_idle :: s_wait_rvalid :: s_wait_ready :: s_wait_control_arready :: s_begin :: s_wait_exu_valid :: s_wait_arready :: Nil = Enum(7)
  //val state = RegInit(s_begin)
  //state := MuxLookup(state, s_idle)(List(
  //  s_begin                -> Mux(io.axi.arready, s_wait_exu_valid, s_begin),
  //  s_wait_exu_valid       -> Mux(io.exu_ifu_raw.exu_valid, Mux(io.axi.arready, s_wait_rvalid, s_wait_arready), s_wait_exu_valid),
  //  s_wait_arready         -> Mux(io.axi.arready, s_wait_rvalid, s_wait_arready),
  //  s_idle                 -> Mux(io.axi.arready, Mux(io.axi.rvalid, s_wait_ready, s_wait_rvalid), s_idle),
  //  s_wait_rvalid          -> Mux(io.axi.rvalid, Mux(io.exu_ifu_raw.isRAW_control, s_wait_control_arready, Mux(io.ifu_idu_data.ready, s_idle, s_wait_ready)), s_wait_rvalid),
  //  s_wait_control_arready -> Mux(io.axi.arready, s_wait_exu_valid, s_wait_control_arready),
  //  s_wait_ready           -> Mux(io.exu_ifu_raw.isRAW_control, s_wait_control_arready, Mux(io.ifu_idu_data.ready, s_idle, s_wait_ready))
  //))
  //
  //io.ifu_idu_data.valid := ((state === s_wait_ready || (state === s_wait_rvalid && io.axi.rvalid && io.ifu_idu_data.ready)) && !io.exu_ifu_raw.isRAW_control) || (state === s_wait_control_arready && io.axi.rvalid && io.ifu_idu_data.ready) || (state === s_wait_exu_valid && io.axi.rvalid)
  //
  //io.ifu_idu_data.bits.inst := Mux(io.axi.rvalid && io.ifu_idu_data.ready, io.axi.rdata, finst)
  //io.ifu_idu_data.bits.pc   := PC
  //
  //PC := Mux(reset.asBool, npc,      
  //      Mux(io.exu_ifu_raw.exu_valid && state === s_wait_exu_valid, io.exu_ifu_raw.dnpc,
  //      Mux((state === s_wait_control_arready && io.axi.arready), io.exu_ifu_raw.dnpc,
  //      Mux(((state === s_wait_ready || (state === s_wait_rvalid && io.axi.rvalid)) && io.ifu_idu_data.ready), preaddr, PC))))
  //
  //araddr := Mux(reset.asBool, npc, 
  //          Mux(state === s_begin, PC, 
  //          Mux(state === s_wait_control_arready, io.exu_ifu_raw.dnpc, 
  //          Mux(state === s_wait_exu_valid && io.exu_ifu_raw.exu_valid, io.exu_ifu_raw.dnpc, PC)))) 


  val flush = Wire(Bool())

  val s_wait_arready :: s_wait_rlast :: s_wait_rlast_flush :: s_wait_ready :: Nil = Enum(4)
  val state = RegInit(s_wait_arready)
  state := MuxLookup(state, s_wait_arready)(List(
    s_wait_arready     -> Mux(io.axi.arready, s_wait_rlast, s_wait_arready),
    s_wait_rlast       -> Mux(io.axi.rlast, Mux(io.ifu_idu_data.ready || flush, s_wait_arready, s_wait_ready), Mux(flush, s_wait_rlast_flush, s_wait_rlast)),
    s_wait_rlast_flush -> Mux(io.axi.rlast, s_wait_arready, s_wait_rlast_flush),
    s_wait_ready       -> Mux(io.ifu_idu_data.ready || flush, s_wait_arready, s_wait_ready)
  ))

  io.ifu_idu_data.valid := (state === s_wait_ready || (state === s_wait_rlast && io.axi.rlast)) && !flush

  io.ifu_idu_data.bits.inst := io.axi.rdata
  io.ifu_idu_data.bits.pc   := PC

  PC := Mux(reset.asBool, npc,      
        Mux(flush, io.exu_ifu_raw.dnpc,
        Mux(io.ifu_idu_data.fire, preaddr, PC)))

  araddr  := Mux(flush, io.exu_ifu_raw.dnpc, PC)
  preaddr := PC + 1.U
  flush   := io.exu_ifu_raw.isRAW_control

  //io.axi.rready  := state === s_idle || state === s_wait_rvalid || state === s_wait_exu_valid || state === s_wait_rvalid || state === s_wait_control_arready || state === s_wait_arready
  //io.axi.arvalid := Mux(reset.asBool, false.B, state === s_idle || state === s_begin || state === s_wait_control_arready || (state === s_wait_exu_valid && io.exu_ifu_raw.exu_valid) || state === s_wait_arready)
  io.axi.araddr  := Cat(araddr, 0.U(2.W))
  io.axi.rready  := state =/= s_wait_ready
  io.axi.arvalid := state === s_wait_arready
  io.axi.awvalid := false.B
  io.axi.awaddr  := 0.U
  io.axi.awid    := "h1".U
  io.axi.awlen   := "h0".U
  io.axi.awsize  := "h2".U
  io.axi.awburst := "h1".U
  io.axi.wvalid  := false.B
  io.axi.wdata   := 0.U
  io.axi.wstrb   := "b11".U
  io.axi.wlast   := false.B
  io.axi.bready  := false.B
  io.axi.arid    := "h1".U
  io.axi.arlen   := "h0".U
  io.axi.arsize  := "h2".U
  io.axi.arburst := "h1".U

  //when(io.axi.rvalid) {
  //  finst := io.axi.rdata
  //}

  // useCounter
  if(Config.useNPCSim) {
    val ifu_counter = Module(new IFU_COUNTER())
    ifu_counter.io.clock   := clock
    ifu_counter.io.state   := state
    ifu_counter.io.araddr  := io.axi.araddr
    ifu_counter.io.arvalid := io.axi.arvalid
  }
}

