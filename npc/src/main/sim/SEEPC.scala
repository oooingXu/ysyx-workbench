package npc

import chisel3._
import chisel3.util._

class SEEPC extends BlackBox with HasBlackBoxInline{
  val io = IO(new Bundle{
    val clock = Input(Clock())
    val valid = Input(Bool())
    val pc    = Input(UInt(Base.addrWidth.W))
    val dnpc  = Input(UInt(Base.addrWidth.W))
    val inst  = Input(UInt(Base.dataWidth.W))
    val wbu_sram_data = new WBU_SRAM_DATA()
  })

  setInline(
    "seepc.sv",
  """`ifdef VERILATOR
    |import "DPI-C" function void pipeline_state(input int pc, input int dnpc, input int inst, input int valid, input int araddr, input int arvalid, input int arsize, input int awaddr, input int wdata, input int awvalid, input int wstrb);
    |`endif
    | module SEEPC(
    |   input clock,
    |   input valid,
    |   input [31:0] pc,
    |   input [31:0] dnpc,
    |   input [31:0] inst,
    |   input [31:0] wbu_sram_data_araddr,
    |   input [31:0] wbu_sram_data_awaddr,
    |   input [31:0] wbu_sram_data_wdata,
    |   input [3:0]  wbu_sram_data_wstrb,
    |   input [2:0]  wbu_sram_data_arsize,
    |   input        wbu_sram_data_arvalid,
    |   input        wbu_sram_data_awvalid
    | );
    | `ifdef VERILATOR
    | always@(posedge clock) begin
    |   pipeline_state(pc, dnpc, inst, {31'b0, valid}, wbu_sram_data_araddr, {31'b0, wbu_sram_data_arvalid}, {29'b0, wbu_sram_data_arsize}, wbu_sram_data_awaddr, wbu_sram_data_wdata, {31'b0, wbu_sram_data_awvalid}, {28'b0, wbu_sram_data_wstrb});
    | end
    |`endif
    |
    | endmodule
  """.stripMargin)
}

