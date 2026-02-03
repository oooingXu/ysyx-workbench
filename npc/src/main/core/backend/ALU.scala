package npc

import chisel3._
import chisel3.util._

class ysyx_23060336_ALU(n: Int) extends Module {
	val io = IO(new Bundle{
		val sel		 = Input(UInt(Base.AluSelWidth.W))
		val ina		 = Input(UInt(n.W))
		val inb		 = Input(UInt(n.W))
		val result = Output(UInt(n.W))
    val mul_valid = Input(Bool())
    val div_valid = Input(Bool())
    val alu_valid = Output(Bool())
    val ready = Input(Bool())
	})

  val zero     = Wire(Bool())
  val carry    = Wire(Bool())
  val overflow = Wire(Bool())
	val cin			 = io.sel(0) | io.sel(3)

	val out1  = Wire(UInt(n.W))
	val out3  = ~io.ina 
	val out4  = io.ina & io.inb
	val out5  = io.ina | io.inb
	val out6  = io.ina ^ io.inb
	val out7  = Wire(UInt(n.W))

  // 独立比较逻辑 
  val a_signed = io.ina.asSInt
  val b_signed = io.inb.asSInt

  // 无符号比较
  val lt_u  = io.ina < io.inb
  val geu_u = io.ina >= io.inb

  // 有符号比较
  val lt_s  = a_signed < b_signed
  val ge_s  = a_signed >= b_signed

  // 相等比较（复用原逻辑）
  val beq = io.ina === io.inb
  val neq = io.ina =/= io.inb

	val addsub = Module(new ysyx_23060336_AddSub(n))
	addsub.io.ina := io.ina
	addsub.io.inb := io.inb
	addsub.io.cin := cin
	carry         := addsub.io.carry
	zero          := addsub.io.zero
	overflow      := addsub.io.overflow
	out1          := addsub.io.result
	
	val shift = Module(new ysyx_23060336_SHIFT(n))
  shift.io.in      := io.ina
  shift.io.shamt   := io.inb
  shift.io.isLeft  := io.sel(0)
  shift.io.izArith := io.sel(1)
  out7             := shift.io.out

  // mul
  val mul = Module(new MyMul(n))
  mul.io.ina := io.ina
  mul.io.inb := io.inb
  mul.io.ready := io.ready
  mul.io.aIsUnSigned := io.sel(1)
  mul.io.bIsUnSigned := io.sel(0)
  mul.io.isHi := io.sel(2)
  mul.io.mul_valid := io.mul_valid
  val mul_out_valid = mul.io.out_valid
  val mul_result = mul.io.out

  // div
  val div = Module(new MyDiv(n))
  div.io.ina := io.ina
  div.io.inb := io.inb
  div.io.signed := !io.sel(0)
  div.io.ready := io.ready
  div.io.div_valid := io.div_valid
  val div_out_valid = div.io.out_valid
  val div_result = Mux(io.sel(2), div.io.rem, div.io.quo)

  // alu_valid
  io.alu_valid := Mux(io.sel(4), mul_out_valid || div_out_valid, true.B)

  val out8  = mul_result
  val out9  = div_result

  io.result := MuxLookup(io.sel, 0.U)(
    Seq(
      0.U  -> out1,   // a + b
      1.U  -> out1,   // a - b
      2.U  -> out3,   // ~a
      3.U  -> out4,   // a & b
      4.U  -> out5,   // a | b
      5.U  -> out6,   // a ^ b
      6.U  -> out7,   // a >> b A
      7.U  -> out7,   // a << b
      8.U  -> out7,   // a >>> b L
      9.U  -> lt_s,   // a < b
      10.U -> lt_u,   // a < b u
      11.U -> ge_s,   // a >= b
      12.U -> geu_u,  // a >= b u
      13.U -> beq,    // a == b
      14.U -> neq,    // a != b
      16.U -> out8,   // MUL
      20.U -> out8,   // MULH
      21.U -> out8,   // MULHU
      23.U -> out8,   // MULHSU
      24.U -> out9,   // DIV
      27.U -> out9,   // DIVU
      28.U -> out9,   // REM
      31.U -> out9,   // REMU
    )
  )
} 

class ysyx_23060336_AddSub(n: Int) extends Module {
	val io = IO(new Bundle{
		val cin			 = Input(Bool())
		val ina			 = Input(UInt(n.W))
		val inb			 = Input(UInt(n.W))
		val result	 = Output(UInt(n.W))
		val zero		 = Output(Bool())
		val carry		 = Output(Bool())
		val overflow = Output(Bool())
	})

  val t_no_cin  = Wire(UInt(32.W))
  val sum       = Wire(UInt(33.W))
	
	t_no_cin := (Fill(n, io.cin) ^ io.inb) 
	sum      := io.ina +& t_no_cin + io.cin

  io.result   := sum(n-1, 0)
  io.carry    := sum(n)
  io.overflow := (io.ina(n-1) === t_no_cin(n-1) && io.result(n-1) =/= io.ina(n-1))
  io.zero     := (io.result === 0.U)
}

