package npc

import chisel3._
import chisel3.util._
import scala.math.max
import chisel3.util.Pipe

class MyDIV(xlen: Int) extends Module {
  val io = IO(new Bundle{
    val ina = Input(UInt(Base.dataWidth.W))
    val inb = Input(UInt(Base.dataWidth.W))
    val signed = Input(Bool())
    val quo = Output(UInt(Base.dataWidth.W))
    val rem = Output(UInt(Base.dataWidth.W))
    val ready = Input(Bool())
    val div_valid = Input(Bool())
    val out_valid = Output(Bool())
  })

  /* stage1: solve sign */
  val sign_s = Mux(io.signed, io.ina(xlen-1) ^ io.inb(xlen-1), false.B)
  val sign_r = Mux(io.signed, io.ina(xlen-1), false.B)
  val src1 = Mux(io.signed && io.ina(xlen-1), ~io.ina + 1.U, io.ina)
  val src2 = Mux(io.signed && io.inb(xlen-1), ~io.inb + 1.U, io.inb)

  // get highest 1 in src1
  val high_rev = PriorityEncoder(Reverse(src1))

  val cnt             = RegInit(0.U(6.W))
  val stage1_fire     = cnt === 0.U

  val src1_reg1       = RegEnable(src1, stage1_fire)
  val src2_reg1       = RegEnable(src2, stage1_fire)
  val signed_reg1     = RegEnable(io.signed, stage1_fire)
  val sign_s_reg1     = RegEnable(sign_s, stage1_fire)
  val sign_r_reg1     = RegEnable(sign_r, stage1_fire)
  val en_reg1         = RegEnable(io.valid, stage1_fire)
  val high_rev_reg1   = RegEnable(high_rev, stage1_fire)

  /* stage2+: div */
  val stage2_init     = en_reg1 && cnt === 0.U

  val src2_reg2       = RegEnable(src2_reg1, stage2_init)
  val signed_reg2     = RegEnable(signed_reg1, stage2_init)
  val sign_s_reg2     = RegEnable(sign_s_reg1, stage2_init)
  val sign_r_reg2     = RegEnable(sign_r_reg1, stage2_init)

  when(cnt =/= 0.U){
      cnt := cnt - 1.U
  }.elsewhen(en_reg1){
      cnt := (xlen+1).U - high_rev_reg1
  }

  val quo_rem_reg = RegInit(0.U((xlen*2+1).W))
  val quo = quo_rem_reg(xlen-1, 0)
  val rem = quo_rem_reg(xlen*2-1, xlen)
  when(cnt =/= 0.U){
      val mins = rem - src2_reg2
      when(rem >= src2_reg2){
          quo_rem_reg := mins(xlen-1, 0) ## quo ## 1.U(1.W)
      }.otherwise{
          quo_rem_reg := quo_rem_reg(xlen*2-1, 0) ## 0.U(1.W)
      }
  }.elsewhen(en_reg1){
      quo_rem_reg := (0.U((xlen+1).W) ## src1_reg1) << high_rev_reg1
  }

  io.quo := Mux(signed_reg2,
      Mux(sign_s_reg2, ~quo + 1.U, quo),
      quo
  )

  val rem_res = quo_rem_reg(xlen*2, xlen+1)
  io.rem := Mux(signed_reg2,
      Mux(sign_r_reg2, ~rem_res + 1.U, rem_res),
      rem_res
  )

  // state machine
  val div_counter = RegInit(0.U(4.W))

  val div_idle :: div_wait :: div_wait_ready :: Nil = Enum(3)
  val state = RegInit(div_idle)
  state := MuxLookup(state, div_idle)(List(
    div_idle -> Mux(io.div_valid, div_wait, div_idle),
    div_wait -> Mux(div_counter === 2.U, div_wait_ready, div_wait),
    div_wait_ready  -> Mux(io.ready, div_idle, div_wait_ready)
  ))

  div_counter := Mux(state === div_wait, div_counter + 1.U, 0.U)
  io.out_valid := state === div_wait_ready 
}

