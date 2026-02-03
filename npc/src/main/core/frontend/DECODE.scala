package npc

import chisel3._
import chisel3.util._
import chisel3.util.BitPat
import chisel3.util.experimental.decode._
import org.chipsalliance.rvdecoderdb

case class InstructionPattern(val inst: rvdecoderdb.Instruction) extends DecodePattern {
  override def bitPat: BitPat = BitPat("b" + inst.encoding.toString())
}

object InstructionPattern {
  implicit class AddMethodsToInsn(i: InstructionPattern) {
    def hasArg(arg: String): Boolean = i.inst.args.map(_.name).contains(arg)
  }
}

object PcMuxField extends DecodeField[InstructionPattern, UInt] {
  override def name = "pcmux"
  override def chiselType = UInt(Base.pcmuxWidth.W)
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "jalr"   => BitPat("b10")   // I-type pc =  imm + src1
    case "jal"    => BitPat("b01")   // J-type pc += imm
    case "ecall" | "mret" => BitPat("b11") // B-type mtvec | mepc
    case _ => BitPat("b00")          // Default PC+4
  }
}

object EcallField extends BoolDecodeField[InstructionPattern] {
  override def name = "ecall"
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "ecall" => BitPat("b1")
    case _       => BitPat("b0")
  }
}

object EbreakField extends BoolDecodeField[InstructionPattern] {
  override def name = "ebreak"
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "ebreak" => BitPat("b1")
    case _        => BitPat("b0")
  }
}

object MretField extends BoolDecodeField[InstructionPattern] {
  override def name = "mret"
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "mret" => BitPat("b1")
    case _      => BitPat("b0")
  }
}

object MemWrField extends BoolDecodeField[InstructionPattern] {
  override def name = "memwr"
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "sb" | "sh" | "sw" => BitPat("b1")
    case _                  => BitPat("b0") 
  }
}

object RegWrField extends BoolDecodeField[InstructionPattern] {
  override def name = "regwr"
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "add" | "sub" | "sll" | "slt" | "sltu" | "xor" | "srl" | "sra" | "or" | "and" | "addi" | "slti" | "slli" | "srli" | "srai" | "sltiu" | "xori" | "ori" | "andi" | "lui" | "auipc" | "jal" | "jalr" | "lb" | "lh" | "lw" | "lbu" | "lhu" | "csrrw" | "csrrs" | "csrrc" | "csrrwi" | "csrrsi" | "csrrci" | "mul" | "mulh" | "mulhu" | "mulhsu" => BitPat("b1")
    case _ => BitPat("b0")
  }
}

object MemtoRegField extends BoolDecodeField[InstructionPattern] {
  override def name = "memtoreg"
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "lb" | "lh" | "lw" | "lbu" | "lhu" => BitPat("b1")
    case _                                  => BitPat("b0")
  }
}

object CsrWrField extends BoolDecodeField[InstructionPattern] {
  override def name = "csrwr"
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "csrrw" | "csrrs" | "csrrc" | "csrrwi" | "csrrsi" | "csrrci" => BitPat("b1")
    case _                                                            => BitPat("b0")
  }
}

object BranchField extends BoolDecodeField[InstructionPattern] {
  override def name = "branch"
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "beq" | "bne" | "blt" | "bge" | "bltu" | "bgeu" => BitPat("b1")
    case _                                               => BitPat("b0")
  }
}

object AluSelField extends DecodeField[InstructionPattern, UInt] {
  override def name = "alusel"
  override def chiselType = UInt(Base.AluSelWidth.W)
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "add"  | "addi" | "csrrw" | "csrrwi" => BitPat("b00000") // +
    case "sub"                                => BitPat("b00001") // -
    case "and"  | "andi" | "csrrc" | "csrrci" => BitPat("b00011") // &
    case "or"   | "ori"  | "csrrs" | "csrrsi" => BitPat("b00100") // |
    case "xor"  | "xori"                      => BitPat("b00101") // ^
    case "sra"  | "srai"                      => BitPat("b00110") // >a
    case "sll"  | "slli"                      => BitPat("b00111") // <
    case "srl"  | "srli"                      => BitPat("b01000") // >l
    case "slt"  | "slti"  | "blt"             => BitPat("b01001") // ><
    case "sltu" | "sltiu" | "bltu"            => BitPat("b01010") // ><u
    case "bge"                                => BitPat("b01011") // >=
    case "bgeu"                               => BitPat("b01100") // >=u
    case "beq"                                => BitPat("b01101") // ==
    case "bne"                                => BitPat("b01110") // !=
    case "mul"                                => BitPat("b01111") // *
    case "mulh"                               => BitPat("b10000") // *
    case "mulhsu"                             => BitPat("b10001") // *
    case "mulhu"                              => BitPat("b10011") // *
    case _                                    => BitPat("b00000") // +
  }
}
    
object RdenField extends BoolDecodeField[InstructionPattern] {
  override def name = "rden"
  override def genTable(i : InstructionPattern): BitPat = {
    val rden = i.inst.args
    .map(_.name match {
      case "rd" => BitPat("b1")
      case _    => BitPat("b0")
    })
    .filterNot(_.value == BitPat("b0").value)
    .headOption
    .getOrElse(BitPat("b0"))

    rden
  }
}


object Rs1enField extends BoolDecodeField[InstructionPattern] {
  override def name = "rs1en"
  override def genTable(i: InstructionPattern): BitPat = {
    val rs1en = i.inst.args
    .map(_.name match {
      case "rs1" => BitPat("b1")
      case _     => BitPat("b0")
    })
    .filterNot(_.value == BitPat("b0").value)
    .headOption
    .getOrElse(BitPat("b0"))

    rs1en
  }
}

object Rs2enField extends BoolDecodeField[InstructionPattern] {
  override def name = "rs2en"
  override def genTable(i: InstructionPattern): BitPat = {
    val rs2en = i.inst.args
    .map(_.name match {
      case "rs2" => BitPat("b1")
      case _     => BitPat("b0")
    })
    .filterNot(_.value == BitPat("b0").value)
    .headOption
    .getOrElse(BitPat("b0"))

    rs2en
  }
}

object ImmTypeField extends DecodeField[InstructionPattern, UInt] {
  override def name = "immtype"
  override def chiselType = UInt(Base.immTypeWidth.W)
  override def genTable(i: InstructionPattern): BitPat = {
    val immType = i.inst.args
    .map(_.name match {
      case "imm12"                 => BitPat("b000") // I
      case "imm12hi"  | "imm12lo"  => BitPat("b001") // S
      case "bimm12hi" | "bimm12lo" => BitPat("b010") // B
      case "imm20"                 => BitPat("b011") // U
      case "jimm20"                => BitPat("b100") // J
      case "shamtw"                => BitPat("b101") // I
      case _                       => BitPat("b111") // R
    })
    .filterNot(_.value == BitPat("b111").value)
    .headOption
    .getOrElse(BitPat("b111"))

    immType
  }
}

object AluMuxField extends DecodeField[InstructionPattern, UInt] {
  override def name = "alumux"
  override def chiselType = UInt(Base.AluMuxWidth.W)
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "lb" | "lh" | "lw" | "lbu" | "lhu" | "sb" | "sh" | "sw" | "addi" | "slti" | "sltiu" | "xori" | "ori" | "andi" | "slli" | "srli" | "srai" | "fence.i" => BitPat("b0001") // src1 imm
    case "beq" | "bne" | "blt" | "bge" | "bltu" | "bgeu" | "add" | "sub" | "sll" | "slt" | "sltu" | "xor" | "srl" | "sra" | "or" | "and" | "mul" | "mulh" | "mulhu" | "mulhsu" => BitPat("b0111") // src1 src2
    case "jal"    | "jalr"   => BitPat("b0010") // pc 4
    case "lui"               => BitPat("b0011") // 0 imm
    case "auipc"             => BitPat("b0100") // pc imm
    case "csrrs"  | "csrrc"  => BitPat("b0101") // src1 C(csr)
    case "csrrsi" | "csrrci" => BitPat("b0110") // zimm C(csr)
    case "csrrw"             => BitPat("b1000") // src1 0
    case "csrrwi"            => BitPat("b1001") // zimm 0
    case _                   => BitPat("b0000")
  }
}
    
object RecsrField extends BoolDecodeField[InstructionPattern] {
  override def name = "recsr"
  override def genTable(i: InstructionPattern): BitPat = i.inst.name match {
    case "csrrc" | "csrrci" => BitPat("b1")
    case _                  => BitPat("b0")
  }
}

class ysyx_23060336_DECODE extends Module {
  val io = IO(new Bundle{
    val decode_idu_data = new DECODE_IDU_DATA()
  })

  // immgen
  val immgen = Module(new ysyx_23060336_IMMGEN())

  val RegWr      = Wire(Bool())
  val MemWr      = Wire(Bool())
  val MemtoReg   = Wire(Bool())
  val CsrWr      = Wire(Bool())
  val recsr      = Wire(Bool())
  val isRAW_data = Wire(Bool())
  val rden       = Wire(Bool())
  val rs1en      = Wire(Bool())
  val rs2en      = Wire(Bool())
  val branch     = Wire(Bool())
  val ecall      = Wire(Bool())
  val ebreak     = Wire(Bool())
  val mret       = Wire(Bool())
  val rd         = Wire(UInt(Base.rdWidth.W))
  val csr        = Wire(UInt(Base.csrWidth.W))
  val immType    = Wire(UInt(Base.immTypeWidth.W))
  val inst       = Wire(UInt(Base.dataWidth.W))
  val func3      = Wire(UInt(3.W))

  val src1    = Wire(UInt(Base.dataWidth.W))
  val src2    = Wire(UInt(Base.dataWidth.W))
  val imm     = Wire(UInt(Base.dataWidth.W))
  val rers1   = Wire(UInt(Base.dataWidth.W))
  val rezimm  = Wire(UInt(Base.dataWidth.W))
  val csrdata = Wire(UInt(Base.dataWidth.W))
  val ina     = Wire(UInt(Base.dataWidth.W))
  val inb     = Wire(UInt(Base.dataWidth.W))
  val AluMux  = Wire(UInt(Base.AluMuxWidth.W))
  val AluSel  = Wire(UInt(Base.AluSelWidth.W))

  val pc      = Wire(UInt(Base.pcWidth.W))
  val mtvec   = Wire(UInt(Base.pcWidth.W))
  val mepc    = Wire(UInt(Base.pcWidth.W))
  val pcmux   = Wire(UInt(Base.pcmuxWidth.W))

  val dnpc_pc_1     = Wire(UInt(Base.pcWidth.W))
  val dnpc_pc_imm   = Wire(UInt(Base.pcWidth.W))
  val dnpc_src1_imm = Wire(UInt(Base.pcWidth.W))

  // rvdecodedb
  val instTable:Iterable[rvdecoderdb.Instruction] = rvdecoderdb.instructions(os.pwd / "rvdecoderdb" / "rvdecoderdbtest" / "jvm" / "riscv-opcodes")

  val rviExceptInstructions      = Set("fence")
  val rv32iExceptInstructions    = Set("slli_rv32", "srli_rv32", "srai_rv32")
  val rvsystemExceptInstructions = Set("wfi")

  val rviTargetSets      = Set("rv_i")
  val rvmTargetSets      = Set("rv_m")
  val rv32iTargetSets    = Set("rv32_i")
  val rvzicsrTargetSets  = Set("rv_zicsr")
  val rvsystemTargetSets = Set("rv_system")
  
  // add implemented instructions here
  val rviInstList = instTable
  .filter(instr => rviTargetSets.contains(instr.instructionSet.name))
  .filter(instr => !rviExceptInstructions.contains(instr.name))
  .filter(_.pseudoFrom.isEmpty)
  .map(InstructionPattern(_))
  .toSeq
  
  val rvmInstList = instTable
  .filter(instr => rvmTargetSets.contains(instr.instructionSet.name))
  .filter(instr => !rviExceptInstructions.contains(instr.name))
  .filter(_.pseudoFrom.isEmpty)
  .map(InstructionPattern(_))
  .toSeq
  
  val rv32iInstList = instTable
  .filter(instr => rv32iTargetSets.contains(instr.instructionSet.name))
  .filter(instr => !rv32iExceptInstructions.contains(instr.name))
  .map(InstructionPattern(_))
  .toSeq
  
  val rvzicsrInstList = instTable
  .filter(_.pseudoFrom.isEmpty)
  .filter(instr => rvzicsrTargetSets.contains(instr.instructionSet.name))
  .map(InstructionPattern(_))
  .toSeq
  
  val rvsystemInstList = instTable
  .filter(_.pseudoFrom.isEmpty)
  .filter(instr => rvsystemTargetSets.contains(instr.instructionSet.name))
  .filter(instr => !rvsystemExceptInstructions.contains(instr.name))
  .map(InstructionPattern(_))
  .toSeq
  
  val instList = rviInstList ++ rvmInstList ++ rv32iInstList ++ rvzicsrInstList ++ rvsystemInstList
  //println(s"The length of instList is: ${instList.length}")

  // decodefield
  val allfield = Seq(PcMuxField, EcallField, EbreakField, MretField, MemWrField, RegWrField, MemtoRegField, CsrWrField, BranchField, AluSelField, AluMuxField, RecsrField, ImmTypeField, RdenField, Rs1enField, Rs2enField) 
  val decodeTable   = new DecodeTable(instList, allfield) 
  val decodeBundle = decodeTable.decode(inst)

  csr      := inst(31, 20)
  func3    := inst(14, 12)

  pc      := io.decode_idu_data.pc
  inst    := io.decode_idu_data.inst
  csrdata := io.decode_idu_data.idu_csr_data.csrdata
  mepc    := io.decode_idu_data.idu_csr_data.mepc
  mtvec   := io.decode_idu_data.idu_csr_data.mtvec

  rd       := Mux(isRAW_data, 0.U, inst(11, 7))
  MemWr    := Mux(isRAW_data, 0.U, decodeBundle(MemWrField))
  MemtoReg := Mux(isRAW_data, 0.U, decodeBundle(MemtoRegField))
  CsrWr    := Mux(isRAW_data, 0.U, decodeBundle(CsrWrField))
  AluMux   := Mux(isRAW_data, "b0011".U, decodeBundle(AluMuxField))
  AluSel   := Mux(isRAW_data, 0.U, decodeBundle(AluSelField))
  pcmux    := Mux(isRAW_data, 0.U, decodeBundle(PcMuxField))

  RegWr   := decodeBundle(RegWrField)
  immType := decodeBundle(ImmTypeField)
  recsr   := decodeBundle(RecsrField)
  rden    := decodeBundle(RdenField)
  rs1en   := decodeBundle(Rs1enField)
  rs2en   := decodeBundle(Rs2enField)
  branch  := decodeBundle(BranchField)
  ecall   := decodeBundle(EcallField)
  ebreak  := decodeBundle(EbreakField)
  mret    := decodeBundle(MretField)

  src1       := immgen.io.immgen_decode_data.src1
  src2       := immgen.io.immgen_decode_data.src2
  imm        := immgen.io.immgen_decode_data.imm
  rers1      := immgen.io.immgen_decode_data.rers1
  rezimm     := immgen.io.immgen_decode_data.rezimm
  isRAW_data := immgen.io.immgen_decode_data.isRAW_data

  dontTouch(mret)
  dontTouch(ecall)
  dontTouch(isRAW_data)

  ina := MuxLookup(AluMux, 0.U)(
    Seq(
      "b0111".U(Base.AluMuxWidth.W) -> src1,
      "b0001".U(Base.AluMuxWidth.W) -> src1,
      "b0010".U(Base.AluMuxWidth.W) -> Cat(pc, 0.U(2.W)),
      "b0011".U(Base.AluMuxWidth.W) -> 0.U,
      "b0100".U(Base.AluMuxWidth.W) -> Cat(pc, 0.U(2.W)),
      "b0101".U(Base.AluMuxWidth.W) -> rers1,
      "b1000".U(Base.AluMuxWidth.W) -> rers1,
      "b1001".U(Base.AluMuxWidth.W) -> rezimm,
      "b0110".U(Base.AluMuxWidth.W) -> rezimm
    )
  )

  inb := MuxLookup(AluMux, 0.U)(
    Seq(
      "b0111".U(Base.AluMuxWidth.W) -> src2,
      "b0001".U(Base.AluMuxWidth.W) -> imm,
      "b0010".U(Base.AluMuxWidth.W) -> 4.U,
      "b0011".U(Base.AluMuxWidth.W) -> imm,
      "b0101".U(Base.AluMuxWidth.W) -> csrdata,
      "b0110".U(Base.AluMuxWidth.W) -> csrdata,
      "b0100".U(Base.AluMuxWidth.W) -> imm
    )
  )

  dnpc_pc_1     := pc + 1.U
  dnpc_pc_imm   := pc + imm(31, 2)
  dnpc_src1_imm := Mux(ecall, mtvec, Mux(mret, mepc, (src1 + imm)(31, 2)))

  // idu <> exu
  io.decode_idu_data.idu_exu_data.pc     := pc
  io.decode_idu_data.idu_exu_data.ina    := ina
  io.decode_idu_data.idu_exu_data.inb    := inb
  io.decode_idu_data.idu_exu_data.AluSel := AluSel
  io.decode_idu_data.idu_exu_data.branch := branch
  io.decode_idu_data.idu_exu_data.pcmux  := pcmux
  io.decode_idu_data.idu_exu_data.mret   := mret
  io.decode_idu_data.idu_exu_data.dnpc_pc_1     := dnpc_pc_1
  io.decode_idu_data.idu_exu_data.dnpc_pc_imm   := dnpc_pc_imm
  io.decode_idu_data.idu_exu_data.dnpc_src1_imm := dnpc_src1_imm

  // idu <> lsu
  io.decode_idu_data.idu_exu_data.idu_lsu_data.func3    := func3
  io.decode_idu_data.idu_exu_data.idu_lsu_data.MemWr    := MemWr
  io.decode_idu_data.idu_exu_data.idu_lsu_data.MemtoReg := MemtoReg
  io.decode_idu_data.idu_exu_data.idu_lsu_data.src2     := src2

  // idu <> wbu
  io.decode_idu_data.idu_exu_data.idu_lsu_data.idu_wbu_data.rd         := rd
  io.decode_idu_data.idu_exu_data.idu_lsu_data.idu_wbu_data.RegWr      := RegWr
  io.decode_idu_data.idu_exu_data.idu_lsu_data.idu_wbu_data.CsrWr      := CsrWr
  io.decode_idu_data.idu_exu_data.idu_lsu_data.idu_wbu_data.ecall      := ecall
  io.decode_idu_data.idu_exu_data.idu_lsu_data.idu_wbu_data.ebreak     := ebreak
  io.decode_idu_data.idu_exu_data.idu_lsu_data.idu_wbu_data.rden       := rden
  io.decode_idu_data.idu_exu_data.idu_lsu_data.idu_wbu_data.isRAW_data := isRAW_data
  io.decode_idu_data.idu_exu_data.idu_lsu_data.idu_wbu_data.csr        := csr
  io.decode_idu_data.idu_exu_data.idu_lsu_data.idu_wbu_data.inst       := inst

  // idu <> reg
  io.decode_idu_data.idu_reg_data <> immgen.io.immgen_decode_data.idu_reg_data

  // idu <> csr
  io.decode_idu_data.idu_csr_data.csr                  := csr
  io.decode_idu_data.idu_exu_data.idu_lsu_data.csrdata := csrdata

  // decode <> immgen
  immgen.io.immgen_decode_data.inst      := inst(31, 7)
  immgen.io.immgen_decode_data.recsr     := recsr
  immgen.io.immgen_decode_data.rs1en     := rs1en
  immgen.io.immgen_decode_data.rs2en     := rs2en
  immgen.io.immgen_decode_data.immType   := immType

  // idu <> immgen
  immgen.io.immgen_decode_data.idu_valid := io.decode_idu_data.idu_valid
  immgen.io.immgen_decode_data.immgen_decode_raw <> io.decode_idu_data.immgen_decode_raw

}

