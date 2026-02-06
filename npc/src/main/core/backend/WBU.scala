package npc

import chisel3._
import chisel3.util._

class ysyx_23060336_WBU extends Module {
  val io = IO(new Bundle{
    val lsu_wbu_data = Flipped(Decoupled(new LSU_WBU_DATA()))
    val wbu_reg_data = new WBU_REG_DATA()
    val wbu_csr_data = new WBU_CSR_DATA()
    val wbu_idu_raw  = new WBU_IDU_RAW()
  })

  // state machine
  //val s_idle :: s_reg :: Nil = Enum(2)
  //val state = RegInit(s_idle)
  //state := MuxLookup(state, s_idle)(List(
  //  s_idle      -> Mux(io.lsu_wbu_data.valid, s_reg, s_idle),
  //  s_reg       -> Mux(io.lsu_wbu_data.valid, s_reg, s_idle),
  //))

  io.lsu_wbu_data.ready := true.B

  // ebreak
  if(Config.useDebug) {
    val ebreak    = Module(new ysyx_23060336_EBREAK())
    ebreak.io.clock      := clock
    ebreak.io.ebreak     := io.lsu_wbu_data.bits.idu_wbu_data.ebreak
  }

  // useCounter
  if(Config.useDebug) {
    val wbu_counter = Module(new WBU_COUNTER())
    wbu_counter.io.clock      := clock
    wbu_counter.io.state      := io.lsu_wbu_data.valid
  }

  // diff pipeline pc/dnpc
  if(Config.useDebug) {
    val seepc = Module(new SEEPC())
    seepc.io.clock := clock
    seepc.io.pc    := Cat(io.lsu_wbu_data.bits.exu_wbu_data.pc, 0.U(2.W))
    seepc.io.dnpc  := Cat(io.lsu_wbu_data.bits.exu_wbu_data.dnpc, 0.U(2.W))
    seepc.io.inst  := io.lsu_wbu_data.bits.idu_wbu_data.inst
    seepc.io.valid := io.lsu_wbu_data.valid && !io.lsu_wbu_data.bits.idu_wbu_data.isRAW_data
    seepc.io.wbu_sram_data <> io.lsu_wbu_data.bits.wbu_sram_data
  }

  // useSram / mem_diff
  if(Config.useDebug) {
    val sram_read = Module(new SRAM_READ())
    sram_read.io.clock := clock
    sram_read.io.inst  := io.lsu_wbu_data.bits.idu_wbu_data.inst
    sram_read.io.wbu_sram_data <> io.lsu_wbu_data.bits.wbu_sram_data
  }

  // wbu <> reg
  io.wbu_reg_data.wen   := io.lsu_wbu_data.bits.idu_wbu_data.RegWr && io.lsu_wbu_data.valid
  io.wbu_reg_data.waddr := io.lsu_wbu_data.bits.idu_wbu_data.rd
  io.wbu_reg_data.wdata := io.lsu_wbu_data.bits.regdata

  // wbu <> csr
  io.wbu_csr_data.wen   := io.lsu_wbu_data.bits.idu_wbu_data.CsrWr && io.lsu_wbu_data.valid
  io.wbu_csr_data.waddr := io.lsu_wbu_data.bits.idu_wbu_data.csr
  io.wbu_csr_data.ecall := io.lsu_wbu_data.bits.idu_wbu_data.ecall
  io.wbu_csr_data.wdata := io.lsu_wbu_data.bits.csrdata
  io.wbu_csr_data.mepc  := io.lsu_wbu_data.bits.exu_wbu_data.pc

  // wbu <> idu
  io.wbu_idu_raw.wbu_rd       := io.lsu_wbu_data.bits.idu_wbu_data.rd
  io.wbu_idu_raw.wbu_rden     := io.lsu_wbu_data.bits.idu_wbu_data.rden
  io.wbu_idu_raw.wbu_regdata  := io.lsu_wbu_data.bits.regdata

  // Konata pipeline tracking
  if(Config.useKonata) {
    val cycle_counter = RegInit(0.U(64.W))
    cycle_counter := cycle_counter + 1.U

    val wbu_valid = RegInit(false.B)
    wbu_valid := io.lsu_wbu_data.valid

    val konata_wbu = Module(new KonataTrackerWBU)
    konata_wbu.io.cycle := cycle_counter
    konata_wbu.io.pc := Cat(io.lsu_wbu_data.bits.exu_wbu_data.pc, 0.U(2.W))
    konata_wbu.io.inst := io.lsu_wbu_data.bits.idu_wbu_data.inst
    konata_wbu.io.valid := wbu_valid
    konata_wbu.io.ready := io.lsu_wbu_data.ready
    konata_wbu.io.state := 0.U
  }
}
