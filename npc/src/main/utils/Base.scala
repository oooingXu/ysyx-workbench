package npc 

object Base {
  // if RV32E 16 else 32
  val regNumber     = 16

  // idu sign
  val pcmuxWidth    = 2
  val instTypeWidth = 3
  val immTypeWidth  = 3
  val RegNumWidth   = 3
  val PCMuxWidth    = 3
  val AluSelWidth   = 4
  val AluMuxWidth   = 4
  // if RV32E 4 else 5
  val rdWidth       = 4
  val csrWidth      = 12
  val pcWidth       = 30

  // axi
  val burstWidth = 2
  val respWidth  = 2
  val sizeWidth  = 3
  val idWidth    = 4
  val wstrbWidth = 4
  val lenWidth   = 8
  val addrWidth  = 32
  val dataWidth  = 32

  // csr
  val mstatusWidth = 2
  val mcauseWidth  = 4

  // icache         512B 128B 64B 32B 16B 8B  4B
  val block = 2 //  2  4 2  4 2 4 2 4 2 4 2 4 2 4
  val index = 2 //  7  5 5  3 4 2 3 1 2 x 1 x x x
}
