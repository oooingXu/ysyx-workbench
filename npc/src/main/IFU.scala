package npc

import chisel3._
import chisel3.util._

class Inst extends Bundle {
  val inst = Output(UInt(32.W))
  val pc   = Output(UInt(32.W))
}

class IFU extends Module{
  val io = IO(new Bundle{
    val clock    = Input(Clock())
    val reset    = Input(Bool())
    val halt     = Input(Bool())
    val out      = Decoupled(new Inst)
    val in       = Flipped(Decoupled(new DATAOut))
    val inst     = Output(UInt(32.W))
  })

  val f_idle :: f_wait_ready :: Nil = Enum(2)

  def npc = "h80000000".U
  val state = RegInit(f_idle)

  state := MuxLookup(state, f_idle)(List(
    f_idle       -> Mux(io.out.valid, f_wait_ready, f_idle),
    f_wait_ready -> Mux(io.out.ready, f_idle, f_wait_ready)
  ))

  io.out.valid := (state === f_wait_ready)
  io.in.ready := (state === f_idle)

  val nifu = Module(new nIFU)
  nifu.io.clock := io.clock
  nifu.io.reset := io.reset
  nifu.io.halt  := io.halt
  nifu.io.Maddr := Mux(io.reset, npc, io.in.bits.dnpc)

  io.out.bits.inst := nifu.io.inst
  io.out.bits.pc := Mux(io.reset, npc, io.in.bits.dnpc)

  io.inst := nifu.io.inst

}

