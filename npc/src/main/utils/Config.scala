package npc

object Config {
  val useNPCSim  = sys.env.get("CONFIG_NPC").getOrElse("n") == "y"
  val useICache  = sys.env.get("CONFIG_ICACHE").getOrElse("n") == "y"
  val useDebug   = sys.env.get("CONFIG_VERILATOR_SIM").getOrElse("n") == "y"
  val useKonata  = sys.env.get("CONFIG_KONATA").getOrElse("n") == "y"

  // isa RV32E
  val RV32E      = sys.env.get("CONFIG_RVE").getOrElse("n") == "y"
}
