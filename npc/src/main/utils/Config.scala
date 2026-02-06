package npc

object Config {
  val useNPCSim  = true
  val useICache  = true
  val useDebug   = true
  val useKonata  = sys.env.get("CONFIG_KONATA").getOrElse("n") == "y"

  // isa RV32E
  val RV32E      = false
}
