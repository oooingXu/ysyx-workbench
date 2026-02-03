package npc

import chisel3._
import chisel3.util._
import scala.math.max

object MULOpType {
    // mul
    // bit encoding: | type (2bit) | isWord(1bit) | opcode(2bit) |
    def mul    = "b01111".U
    def mulh   = "b10000".U
    def mulhsu = "b10001".U
    def mulhu  = "b10011".U

    def isSign(op: UInt) = op(0) ^ op(4)
    def isW(op: UInt) = true.B
    def isH(op: UInt) = op(4)
}

class MyMul(xlen: Int) extends Module {
  val io = IO(new Bundle{
    val ina = Input(UInt(Base.dataWidth.W))
    val inb = Input(UInt(Base.dataWidth.W))
    val out = Output(UInt(Base.dataWidth.W))
    val sel = Input(UInt(Base.AluSelWidth.W))
    val ready = Input(Bool())
    val mul_valid = Input(Bool())
    val out_valid = Output(Bool())
  })

  val func = io.sel
  val isHi = MULOpType.isH(func)

  val aIsUnSigned = func === "b10011".U
  val bIsUnSigned = !MULOpType.isSign(func)

  val ina = RegInit(0.U((2*xlen).W))
  val inb = RegInit(0.U((2*xlen).W))

  val inanext = RegNext(ina)
  val inbnext = RegNext(inb)

  ina := Mux(aIsUnSigned, Cat(0.U(xlen.W), io.ina), Cat(Fill(xlen, io.ina(31)), io.ina))
  inb := Mux(bIsUnSigned, Cat(0.U(xlen.W), io.inb), Cat(Fill(xlen, io.inb(31)), io.inb))

  val result = inanext * inbnext
  val out = RegInit(0.U(Base.dataWidth.W))

  out := Mux(isHi, result(63, 32), result(31, 0))
  io.out := out

  dontTouch(ina)
  dontTouch(inb)
  dontTouch(isHi)

  val mul_counter = RegInit(0.U(4.W))

  val mul_idle :: mul_wait :: mul_wait_ready :: Nil = Enum(3)
  val state = RegInit(mul_idle)
  state := MuxLookup(state, mul_idle)(List(
    mul_idle -> Mux(io.mul_valid, mul_wait, mul_idle),
    mul_wait -> Mux(mul_counter === 1.U, mul_wait_ready, mul_wait),
    mul_wait_ready  -> Mux(io.ready, mul_idle, mul_wait_ready)
  ))

  mul_counter := Mux(state === mul_wait, mul_counter + 1.U, 0.U)
  io.out_valid := state === mul_wait_ready
}

