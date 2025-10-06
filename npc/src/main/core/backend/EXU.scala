package npc

import chisel3._
import chisel3.util._

class ysyx_23060336_EXU extends Module {
  val io = IO(new Bundle{
    val idu_exu_data = Flipped(Decoupled(new IDU_EXU_DATA()))
    val exu_lsu_data = Decoupled(new EXU_LSU_DATA())
    val exu_ifu_raw  = new EXU_IFU_RAW()
    val exu_idu_raw  = new EXU_IDU_RAW()
  })

  val pc            = Wire(UInt(Base.pcWidth.W))
  val dnpc          = Wire(UInt(Base.pcWidth.W))
  val dnpc_s        = Wire(UInt(Base.pcWidth.W))
  val dnpc_pc_1     = Wire(UInt(Base.pcWidth.W))
  val dnpc_pc_imm   = Wire(UInt(Base.pcWidth.W))
  val dnpc_src1_imm = Wire(UInt(Base.pcWidth.W))

  val PCMux         = Wire(UInt(Base.PCMuxWidth.W))
  val pcmux         = Wire(UInt(Base.pcmuxWidth.W))
  val result        = Wire(UInt(Base.dataWidth.W))
  val branch        = Wire(Bool())
  val isRAW_control = Wire(Bool())

  // state machine
  val s_idle :: s_wait_ready :: s_wait_ready_flush :: Nil = Enum(3)
  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(List(
    s_idle             -> Mux(io.idu_exu_data.valid, s_wait_ready, s_idle),
    s_wait_ready       -> Mux(io.exu_lsu_data.ready, Mux(isRAW_control, s_idle, Mux(io.idu_exu_data.valid, s_wait_ready, s_idle)), Mux(isRAW_control, s_wait_ready_flush, s_wait_ready)),
    s_wait_ready_flush -> Mux(io.exu_lsu_data.ready, Mux(io.idu_exu_data.valid, s_wait_ready, s_idle), s_wait_ready_flush)
  ))

  io.exu_lsu_data.valid := state === s_wait_ready || state === s_wait_ready_flush
  io.idu_exu_data.ready := state === s_idle || ((state === s_wait_ready || state === s_wait_ready_flush) && io.exu_lsu_data.ready) 

  // exu <> lsu
  io.exu_lsu_data.bits.idu_lsu_data  <> io.idu_exu_data.bits.idu_lsu_data
  io.exu_lsu_data.bits.exu_wbu_data.pc   := io.idu_exu_data.bits.pc
  io.exu_lsu_data.bits.exu_wbu_data.dnpc := dnpc
  io.exu_lsu_data.bits.result := result

  // signal
  branch  := io.idu_exu_data.bits.branch
  pcmux   := io.idu_exu_data.bits.pcmux
  pc      := io.idu_exu_data.bits.pc
  result  := io.idu_exu_data.bits.result

  dnpc_pc_1     := io.idu_exu_data.bits.dnpc_pc_1
  dnpc_pc_imm   := io.idu_exu_data.bits.dnpc_pc_imm
  dnpc_src1_imm := io.idu_exu_data.bits.dnpc_src1_imm

  // exu <> pc_add
  PCMux := Cat(branch && result(0), pcmux)
  dnpc_s := MuxLookup(PCMux, dnpc_pc_1)(
    Seq(
      "b001".U(3.W) -> dnpc_pc_imm,
      "b010".U(3.W) -> dnpc_src1_imm,
      "b011".U(3.W) -> dnpc_src1_imm,
      "b100".U(3.W) -> dnpc_pc_imm,
      )
  )
  dontTouch(dnpc_s)

  dnpc := dnpc_s

  io.exu_ifu_raw.dnpc := dnpc
  isRAW_control := dnpc_pc_1 =/= dnpc && state === s_wait_ready

  // exu_idu_raw
  io.exu_idu_raw.exu_regdata   := result
  io.exu_idu_raw.exu_rd        := io.idu_exu_data.bits.idu_lsu_data.idu_wbu_data.rd
  io.exu_idu_raw.exu_rden      := io.idu_exu_data.bits.idu_lsu_data.idu_wbu_data.rden
  io.exu_idu_raw.exu_MemtoReg  := io.idu_exu_data.bits.idu_lsu_data.MemtoReg
  io.exu_idu_raw.exu_isRAW_control := isRAW_control

  // exu_ifu_raw
  io.exu_ifu_raw.exu_valid     := state === s_wait_ready
  io.exu_ifu_raw.isRAW_control := isRAW_control

  // useCounter
  if(Config.useDebug) {
    val exu_counter = Module(new EXU_COUNTER())
    exu_counter.io.clock         := clock
    exu_counter.io.state         := state
    exu_counter.io.isRAW_control := isRAW_control

    val pc_debug = Wire(UInt(Base.addrWidth.W))
    val dnpc_debug = Wire(UInt(Base.addrWidth.W))

    pc_debug := Cat(pc, 0.U(2.W))
    dnpc_debug := Cat(dnpc, 0.U(2.W))
    
    dontTouch(pc_debug)
    dontTouch(dnpc_debug)
  }

}
