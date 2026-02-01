package npc

import chisel3._
import chisel3.util._

class ysyx_23060336_CSR extends Module{
  val io = IO(new Bundle{
    val csr_idu_data = new CSR_IDU_DATA()
    val csr_wbu_data = new CSR_WBU_DATA()
  })

  def MVENDORID = "hf11".U
  def MARCHID   = "hf12".U
  def MSTATUS   = "h300".U
  def MIE       = "h304".U
  def MTVEC     = "h305".U
  def MSCRATCH  = "h340".U
  def MEPC      = "h341".U
  def MCAUSE    = "h342".U
  def MTVAL     = "h343".U
  def MIP       = "h344".U

  val mstatus   = RegInit("h0".U(Base.mstatusWidth.W))
  val mcause    = RegInit("h0".U(Base.mcauseWidth.W))
  val mtvec     = RegInit("h0".U(Base.pcWidth.W))
  val mepc      = RegInit("h0".U(Base.pcWidth.W))
  //val mie       = RegInit("h0".U(Base.dataWidth.W))
  //val mscratch  = RegInit("h0".U(Base.dataWidth.W))
  //val mtval     = RegInit("h0".U(Base.dataWidth.W))
  //val mip       = RegInit("h0".U(Base.dataWidth.W))

  // mtvec
  mtvec   := Mux(io.csr_wbu_data.wen && io.csr_wbu_data.waddr === MTVEC, io.csr_wbu_data.wdata(31, 2), mtvec)

  // mstatus
  mstatus := Mux(io.csr_wbu_data.wen && io.csr_wbu_data.waddr === MSTATUS, io.csr_wbu_data.wdata(12, 11), 
             Mux(io.csr_wbu_data.ecall, "h3".U, mstatus))

  // mepc
  mepc    := Mux(io.csr_wbu_data.wen && io.csr_wbu_data.waddr === MEPC, io.csr_wbu_data.wdata(31, 2), 
             Mux(io.csr_wbu_data.ecall, io.csr_wbu_data.mepc, mepc))

  // mcause
  mcause  := Mux(io.csr_wbu_data.wen && io.csr_wbu_data.waddr === MCAUSE, io.csr_wbu_data.wdata(3, 0), 
             Mux(io.csr_wbu_data.ecall, "hb".U, mcause))

  io.csr_idu_data.mepc    := mepc
  io.csr_idu_data.mtvec   := mtvec

  io.csr_idu_data.csrdata := MuxLookup(io.csr_idu_data.csr, 0.U)(
    Seq(
      MEPC      -> Cat(mepc, 0.U(2.W)),
      MCAUSE    -> Cat(0.U(28.W), mcause),
      MSTATUS   -> Cat(0.U(19.W), mstatus, 0.U(11.W)),
      MTVEC     -> Cat(mtvec, 0.U(2.W)),
      MVENDORID -> "h79737978".U,
      MARCHID   -> "h15fdf70".U
    )
  )
}

