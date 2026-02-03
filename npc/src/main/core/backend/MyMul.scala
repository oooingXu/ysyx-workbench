package npc

import chisel3._
import chisel3.util._
import scala.math.max
import chisel3.util.Pipe

class MyMul(xlen: Int) extends Module {
  val io = IO(new Bundle{
    val ina = Input(UInt(Base.dataWidth.W))
    val inb = Input(UInt(Base.dataWidth.W))
    val out = Output(UInt(Base.dataWidth.W))
    val aIsUnSigned = Input(Bool())
    val bIsUnSigned = Input(Bool())
    val isHi = Input(Bool())
    val ready = Input(Bool())
    val mul_valid = Input(Bool())
    val out_valid = Output(Bool())
  })

  // 1. 输入寄存器（第1拍）
  val ina_reg = RegEnable(io.ina, 0.U, io.mul_valid)
  val inb_reg = RegEnable(io.inb, 0.U, io.mul_valid)
  val valid_p1 = RegNext(io.mul_valid, false.B)

  // 2. 符号扩展和准备操作数（第2拍）
  val aIsUnSigned = io.aIsUnSigned
  val bIsUnSigned = io.bIsUnSigned
  val isHi = io.isHi

  val a_ext = Mux(aIsUnSigned,
                  Cat(0.U(xlen.W), ina_reg),
                  Cat(Fill(xlen, ina_reg(xlen-1)), ina_reg))
  val b_ext = Mux(bIsUnSigned,
                  Cat(0.U(xlen.W), inb_reg),
                  Cat(Fill(xlen, inb_reg(xlen-1)), inb_reg))

  val a_ext_reg = RegEnable(a_ext, 0.U, valid_p1)
  val b_ext_reg = RegEnable(b_ext, 0.U, valid_p1)
  val isHi_reg = RegEnable(isHi, false.B, valid_p1)
  val valid_p2 = RegNext(valid_p1, false.B)

  // 3. 乘法计算（关键路径，可以考虑分割）
  // 使用 Chisel 的乘法操作，综合工具会自动优化
  val mul_result = a_ext_reg * b_ext_reg

  // 4. 结果寄存器（第3拍）
  val result_reg = RegEnable(mul_result, 0.U, valid_p2)
  val isHi_reg2 = RegEnable(isHi_reg, false.B, valid_p2)
  val valid_p3 = RegNext(valid_p2, false.B)

  // 5. 输出选择（第4拍）
  val out_selected = Mux(isHi_reg2, result_reg(2*xlen-1, xlen), result_reg(xlen-1, 0))
  val out_reg = RegEnable(out_selected, 0.U, valid_p3)
  val valid_out = RegNext(valid_p3, false.B)
  dontTouch(valid_out)

  io.out := out_reg

  // state machine
  val mul_counter = RegInit(0.U(4.W))

  val mul_idle :: mul_wait :: mul_wait_ready :: Nil = Enum(3)
  val state = RegInit(mul_idle)
  state := MuxLookup(state, mul_idle)(List(
    mul_idle -> Mux(io.mul_valid, mul_wait, mul_idle),
    mul_wait -> Mux(mul_counter === 2.U, mul_wait_ready, mul_wait),
    mul_wait_ready  -> Mux(io.ready, mul_idle, mul_wait_ready)
  ))

  mul_counter := Mux(state === mul_wait, mul_counter + 1.U, 0.U)
  io.out_valid := state === mul_wait_ready 
}

