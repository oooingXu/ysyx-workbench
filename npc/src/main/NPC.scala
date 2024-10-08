package npc

import chisel3._
import chisel3.util._
import chisel3.util.HasBlackBoxInline
import chisel3.util.experimental.loadMemoryFromFile

/**
  * Compute GCD using subtraction method.
  * Subtracts the smaller from the larger until register y is zero.
  * value in register x is then the GCD
  */
class NPC extends Module {
	val io = IO(new Bundle{
    val reset  = Input(Bool())
    val clock  = Input(Clock())
    val NPC    = Output(UInt(32.W))
    val PC     = Output(UInt(32.W))
    val inst   = Output(UInt(32.W))
    val halt   = Output(UInt(32.W))
    val mcause = Output(UInt(32.W))
    val mstatus= Output(UInt(32.W))
    val stop   = Output(Bool())
		})

  val stop = RegInit("b1".U(1.W))

  val ifu  = Module(new IFU())
  val idu  = Module(new IDU())
  val exu  = Module(new EXU())
  val lsu  = Module(new LSU())
  val wbu  = Module(new WBU())
  val reg  = Module(new Reg())
  val csr  = Module(new Csr())

  def pipelineConnect[T <: Data, T2 <: Data](prevOut: DecoupledIO[T], thisIn: DecoupledIO[T], thisOut: DecoupledIO[T2]) = {
    prevOut.ready := thisIn.ready
    thisIn.bits   := RegEnable(prevOut.bits, prevOut.valid && thisIn.ready)
    thisIn.valid   := prevOut.valid && thisIn.ready
  }

  pipelineConnect(ifu.io.out, idu.io.in, idu.io.out)
  pipelineConnect(idu.io.out, exu.io.in, exu.io.out)
  pipelineConnect(exu.io.out, lsu.io.in, lsu.io.out)
  pipelineConnect(lsu.io.out, wbu.io.in, wbu.io.out)
  pipelineConnect(wbu.io.out, ifu.io.in, ifu.io.out)

  reg.io.raddr1 := idu.io.rs1
  reg.io.raddr2 := idu.io.rs2

  idu.io.src1   := reg.io.rdata1
  idu.io.src2   := reg.io.rdata2

  idu.io.Csr    := csr.io.rdata

  reg.io.wen    := wbu.io.RegWr
  reg.io.waddr  := wbu.io.rd
  reg.io.wdata  := wbu.io.DataOut

  csr.io.raddr  := idu.io.csr

  csr.io.wdata  := wbu.io.result
  csr.io.waddr  := wbu.io.csr
  csr.io.wen    := wbu.io.CsrWr
  io.mcause     := csr.io.mcause
  io.mstatus    := csr.io.mstatus

  ifu.io.clock  := io.clock
  ifu.io.reset  := io.reset
  ifu.io.halt   := idu.io.halt
  io.inst       := ifu.io.inst

  exu.io.reset  := io.reset
  exu.io.mepc   := csr.io.mepc
  exu.io.mtvec  := csr.io.mtvec
  io.PC         := exu.io.pc
  io.NPC        := exu.io.dnpc

  lsu.io.clock  := io.clock


  stop          := !idu.io.halt
  io.halt       := idu.io.halt

  io.stop       := stop



  val dpiebreak = Module(new DpiEbreak)
  dpiebreak.io.isbreak := exu.io.dnpc
  dpiebreak.io.clock   := io.clock
  dpiebreak.io.stop    := stop
}

class DpiEbreak extends BlackBox with HasBlackBoxInline{
  val io = IO(new Bundle{
    val clock   = Input(Clock())
    val stop    = Input(Bool())
    val isbreak = Input(UInt(32.W))
  })

  setInline(
    "ebreak.sv",
    """import "DPI-C" function void set_npc_state(input int isbreak);
      | module DpiEbreak(
      |   input clock,
      |   input stop,
      |   input [31:0] isbreak
      | );
      |
      | always@(posedge clock) begin
      | if(stop) begin
      |     set_npc_state(isbreak);
      |   end
      | end
      |
      | endmodule
    """.stripMargin)
}


  /*
  StageConnect(ifu.io.out, idu.io.in)
  StageConnect(idu.io.out, exu.io.in)
  StageConnect(exu.io.out, wbu.io.in)
  */

/*
object StageConnect{
  def apply[T <: Data](left: DecoupledIO[T], right: DecoupledIO[T]) = {
  val arch = "multi"

  if      (arch == "single")   { right.bits := left.bits }
  else if (arch == "multi")    { right <> left }
  else if (arch == "pipeline") { right <> RegEnable(left, left.fire) }
  else if (arch == "ooo")      { right <> Queue(left, 16) }

 }
}
*/

