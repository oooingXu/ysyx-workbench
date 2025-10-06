package npc

import chisel3._
import chisel3.util._

// have something wrong
class ysyx_23060336_ICACHE(m: Int, n: Int) extends Module{
  val io = IO(new Bundle{
    val master = new ysyx_23060336_AXI4Master()
    val slave  = new ysyx_23060336_AXI4Slave()
    val coherence_input = new COHERENCE_INPUT()
  })

  val icache_ifu = Module(new ysyx_23060336_ICACHE_IFU())
  val icache_lsu = Module(new ysyx_23060336_ICACHE_LSU(m, n))
  val icache_issue = Module(new ysyx_23060336_ICACHE_ISSUE())

  val slave_araddr = Wire(UInt(Base.pcWidth.W))
  val araddr       = Reg(UInt(Base.pcWidth.W))

  araddr := Mux(io.slave.arvalid, io.slave.araddr(31, 2), araddr)
  slave_araddr := Mux(io.slave.arvalid, io.slave.araddr(31, 2), araddr)

  // icache pipeline
  //def icacheConnect[T <: Data, T2 <: Data](prevOut: DecoupledIO[T], thisIn: DecoupledIO[T]) = {
  //  prevOut.ready := thisIn.ready
  //  thisIn.bits   := RegEnable(prevOut.bits, prevOut.valid && thisIn.ready)
  //  thisIn.valid  := prevOut.valid
  //}

  //icacheConnect(icache_ifu.io.out, icache_lsu.io.in)
  //icacheConnect(icache_lsu.io.out, icache_issue.io.in)

  icache_ifu.io.out <> icache_lsu.io.in
  icache_lsu.io.out <> icache_issue.io.in

  val sram_start = "h03c00000".U(Base.pcWidth.W)
  val sram_end   = "h04000000".U(Base.pcWidth.W)

  val skip_addr  = slave_araddr >= sram_start && slave_araddr <= sram_end

  // state machine
  val s_idle :: s_skip_rlast :: Nil = Enum(2)
  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(List(
    s_idle       -> Mux(io.slave.arvalid && skip_addr && io.master.arready, s_skip_rlast, s_idle),
    s_skip_rlast   -> Mux(io.master.rlast, s_idle, s_skip_rlast),
  ))

  // icache <> arbiter
  io.master <> io.slave

  // ifu <> icache
  io.slave.rdata    := Mux(icache_issue.io.icache_issue, icache_issue.io.icache_out_data, io.master.rdata)
  io.slave.rvalid   := icache_issue.io.icache_issue || io.master.rvalid
  io.slave.rlast    := icache_issue.io.icache_issue || io.master.rlast
  io.slave.arready  := (icache_ifu.io.in.arready && !skip_addr) || (io.master.arready && skip_addr)

  // icache <> icache_ifu
  icache_ifu.io.in.arvalid := io.slave.arvalid && !skip_addr
  icache_ifu.io.in.coherence_input := io.coherence_input
  icache_ifu.io.in.araddr  := slave_araddr

  // arbiter <> icache_lsu
  io.master.araddr  := Mux(state === s_idle, Cat(slave_araddr, 0.U(2.W)), Cat(icache_lsu.io.lsu_arbiter.araddr, 0.U(2.W)))
  io.master.arvalid := (icache_lsu.io.lsu_arbiter.arvalid && !skip_addr) || (io.slave.arvalid && skip_addr)
  io.master.rready  := (icache_lsu.io.lsu_arbiter.rready && !skip_addr) || (io.slave.rready && skip_addr)
  io.master.arlen   := icache_lsu.io.lsu_arbiter.arlen

  icache_lsu.io.lsu_arbiter.rvalid     := io.master.rvalid
  icache_lsu.io.lsu_arbiter.rlast      := io.master.rlast
  icache_lsu.io.lsu_arbiter.rdata      := io.master.rdata
  icache_lsu.io.lsu_arbiter.arready    := io.master.arready
  //icache_lsu.io.lsu_arbiter.ifu_araddr := slave_araddr

  // icache <> icache_issue
  icache_issue.io.ifu_rready := io.slave.rready

  // icache_counter
  if(Config.useDebug) {
    val icache_counter = Module(new ICACHE_COUNTER())
    icache_counter.io.clock          := clock
    icache_counter.io.slave_arvalid  := icache_lsu.io.in.valid
    icache_counter.io.slave_rvalid   := icache_lsu.io.out.valid
    icache_counter.io.master_arvalid := io.master.arvalid
    icache_counter.io.master_rvalid  := io.master.rvalid
  }

}

