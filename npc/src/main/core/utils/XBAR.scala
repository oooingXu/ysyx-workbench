package npc

import chisel3._
import chisel3.util._

class ysyx_23060336_XBAR extends Module{
  val io = IO(new Bundle{
    val slave  = new ysyx_23060336_AXI4Slave()
    val master = new ysyx_23060336_AXI4Master()
    val clint  = new ysyx_23060336_AXI4Master()
  })

  //def CLINT_START = "h02000000".U
  //def CLINT_END   = "h0200ffff".U

  def CLINT_START = "h800000".U
  def CLINT_END   = "h803fff".U

  io.slave <> io.clint
  io.slave <> io.master

  io.master.arid := 0.U
  io.master.awid := 0.U

  when(io.slave.araddr(31, 2) >= CLINT_START && io.slave.araddr(31, 2) <= CLINT_END) {
    io.clint.arvalid  := io.slave.arvalid
    io.slave.arready  := io.clint.arready
    io.slave.rlast    := io.clint.rlast
    io.slave.rvalid   := io.clint.rvalid
    io.slave.rdata    := io.clint.rdata
    io.slave.rresp    := io.clint.rresp
    io.master.arvalid := false.B
  } .otherwise {
    io.master.arvalid := io.slave.arvalid
    io.slave.arready  := io.master.arready
    io.slave.rlast    := io.master.rlast
    io.slave.rvalid   := io.master.rvalid
    io.slave.rdata    := io.master.rdata
    io.slave.rresp    := io.master.rresp
    io.clint.arvalid  := false.B
  }

}

