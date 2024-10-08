package npc

import chisel3._
import chisel3.util._

class MyData extends Bundle{
  val DataOut  = Output(UInt(32.W))
  val result   = Output(UInt(32.W))
  val dnpc     = Output(UInt(32.W))
  val csr      = Output(UInt(12.W))
  val Csr      = Output(UInt(32.W))
  val rd       = Output(UInt(5.W))
  val RegNum   = Output(UInt(3.W))
  val CsrWr    = Output(Bool())
  val RegWr    = Output(Bool())
}

class LSU extends Module{
  val io = IO(new Bundle{
    val clock    = Input(Clock())
    val out      = Decoupled(new MyData)
    val in       = Flipped(Decoupled(new Result))
  })

  val l_idle :: l_wait_ready :: Nil = Enum(2)
  val state = RegInit(l_idle)

  state := MuxLookup(state, l_idle)(List(
    l_idle       -> Mux(io.out.valid, l_wait_ready, l_idle),
    l_wait_ready -> Mux(io.out.ready, l_idle, l_wait_ready)
  ))

  io.out.valid := (state === l_wait_ready)
  io.in.ready := (state === l_idle)

  io.out.bits.result   := io.in.bits.result    
  io.out.bits.dnpc     := io.in.bits.dnpc    
  io.out.bits.csr      := io.in.bits.csr
  io.out.bits.Csr      := io.in.bits.Csr
  io.out.bits.rd       := io.in.bits.rd
  io.out.bits.RegNum   := io.in.bits.RegNum  
  io.out.bits.CsrWr    := io.in.bits.CsrWr
  io.out.bits.RegWr    := io.in.bits.RegWr   

  val elsu = Module(new eLSU)
  elsu.io.clock    := io.clock
  elsu.io.Maddr    := io.in.bits.result
  elsu.io.DataIn   := io.in.bits.src2
  elsu.io.wmask    := io.in.bits.MemNum
  elsu.io.MemWr    := io.in.bits.MemWr
  elsu.io.MemtoReg := io.in.bits.MemtoReg
  io.out.bits.DataOut := Mux(io.in.bits.MemtoReg, elsu.io.DataOut, io.in.bits.result)

}

