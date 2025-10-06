package npc

import chisel3._
import chisel3.util._

class ysyx_23060336_ICACHE_METADATA(m: Int, n: Int) extends Module {
  val io = IO(new Bundle {
    val wen       = Input(Bool())
    val in_index  = Input(UInt(n.W))
    val in_data   = Input(UInt(32.W))
    val in_tag    = Input(UInt((32- m - n).W))
    val in_valid  = Input(Bool())
    val out_data  = Output(UInt(32.W))
    val out_tag   = Output(UInt((32 - m - n).W))
    val out_valid = Output(Bool())
  })

    val ysyx_23060336_tag   = SyncReadMem(1 << n, UInt((32 - m - n).W))
    val ysyx_23060336_data  = SyncReadMem(1 << n, UInt(Base.dataWidth.W))
    val ysyx_23060336_valid = SyncReadMem(1 << n, UInt(1.W))

    // 有效位更新
    when(io.wen) {
      ysyx_23060336_data(io.in_index)  := io.in_data
      ysyx_23060336_valid(io.in_index) := io.in_valid
      ysyx_23060336_tag(io.in_index)   := io.in_tag
    }

    // 输出逻辑
    io.out_tag   := ysyx_23060336_tag(io.in_index)
    io.out_valid := ysyx_23060336_valid(io.in_index)  
    io.out_data  := ysyx_23060336_data(io.in_index)
}

