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

  val slave_arready = Reg(UInt(Base.validWidth.W))
  val slave_rlast   = Reg(UInt(Base.validWidth.W))
  val slave_rvalid  = Reg(UInt(Base.validWidth.W))
  val slave_rresp   = Reg(UInt(Base.respWidth.W))
  val slave_rdata   = Reg(UInt(Base.dataWidth.W))

  val in_clint = Wire(Bool())

  io.slave <> io.clint
  io.slave <> io.master

  io.master.arid := 0.U
  io.master.awid := 0.U

  in_clint := io.slave.araddr(31, 2) >= CLINT_START && io.slave.araddr(31, 2) <= CLINT_END

  io.slave.arready := slave_arready
  io.slave.rlast  := slave_rlast
  io.slave.rvalid := slave_rvalid
  io.slave.rdata  := slave_rdata
  io.slave.rresp  := slave_rresp

  slave_arready := Mux(in_clint, io.clint.arready, Mux(!in_clint, io.master.arready, slave_arready))
  slave_rdata   := Mux(in_clint && io.clint.rvalid, io.clint.rdata, Mux(!in_clint && io.master.rvalid, io.master.rdata, slave_rdata))
  slave_rvalid  := Mux(in_clint, io.clint.rvalid, Mux(!in_clint, io.master.rvalid, slave_rvalid))
  slave_rlast   := Mux(in_clint, io.clint.rlast,  Mux(!in_clint, io.master.rlast,  slave_rlast))
  slave_rresp   := Mux(in_clint, io.clint.rresp,  Mux(!in_clint, io.master.rresp,  slave_rresp))

  when(in_clint) {
    io.clint.arvalid  := io.slave.arvalid
    io.master.arvalid := false.B
  } .otherwise {
    io.master.arvalid := io.slave.arvalid
    io.clint.arvalid  := false.B
  }

}

