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
  def MTVEC     = "h305".U
  def MEPC      = "h341".U
  def MCAUSE    = "h342".U

  val mvendorid = RegInit("h79737978".U(Base.dataWidth.W))
  val marchid   = RegInit("h15fdf70".U(Base.dataWidth.W))
  val mstatus   = RegInit("h3".U(Base.mstatusWidth.W))
  val mcause    = RegInit("h0".U(Base.mcauseWidth.W))
  val mtvec     = RegInit("h0".U(Base.pcWidth.W))
  val mepc      = RegInit("h0".U(Base.pcWidth.W))

  when(io.csr_wbu_data.ecall) {
    mcause  := "hb".U
    mepc    := io.csr_wbu_data.mepc
    mstatus := "h3".U
  }

  when(io.csr_wbu_data.wen){
    when(io.csr_wbu_data.waddr === MTVEC){
      mtvec := io.csr_wbu_data.wdata(31, 2)
    } .elsewhen(io.csr_wbu_data.waddr === MSTATUS) {
      mstatus := io.csr_wbu_data.wdata(12, 11)
    } .elsewhen(io.csr_wbu_data.waddr === MEPC) {
      mepc := io.csr_wbu_data.wdata(31, 2)
    } .elsewhen(io.csr_wbu_data.waddr === MCAUSE) {
      mcause := io.csr_wbu_data.wdata(3, 0)
    }
  }

  io.csr_idu_data.mepc    := mepc
  io.csr_idu_data.mtvec   := mtvec
  io.csr_idu_data.csrdata := Mux(io.csr_idu_data.csr === MEPC, Cat(mepc, 0.U(2.W)), Mux(io.csr_idu_data.csr === MCAUSE, Cat(0.U(28.W), mcause), Mux(io.csr_idu_data.csr === MSTATUS, Cat(0.U(19.W), mstatus, 0.U(11.W)), Mux(io.csr_idu_data.csr === MTVEC, Cat(mtvec, 0.U(2.W)), Mux(io.csr_idu_data.csr === MVENDORID, mvendorid, Mux(io.csr_idu_data.csr === MARCHID, marchid, 0.U))))))

  //io.csr_idu_data.csrdata := MuxLookup(io.csr_idu_data.csr, 0.U)(
  //  Seq(
  //    MEPC      -> Cat(mepc, 0.U(2.W)),
  //    MCAUSE    -> Cat(0.U(28.W), mcause),
  //    MSTATUS   -> Cat(0.U(19.W), mstatus, 0.U(11.W)),
  //    MTVEC     -> Cat(mtvec, 0.U(2.W)),
  //    MVENDORID -> mvendorid,
  //    MARCHID   -> marchid
  //  )
  //)
}

