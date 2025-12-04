#include <cpu/cpu.h>
#include <cpu/trace.h>
#include <isa.h>

uint32_t itrace_pc = 0;

enum {
  TYPE_I, TYPE_U, TYPE_S,TYPE_R, TYPE_J, TYPE_B,
  TYPE_N, // none
};

const static char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

const static char *csrs[4096] = {
[0x300] = "mstatus",
[0x301] = "misa",
[0x302] = "medeleg",
[0x303] = "mideleg",
[0x304] = "mie",
[0x305] = "mtvec",
[0x306] = "mcounteren",
[0x340] = "mscratch",
[0x341] = "mepc",
[0x342] = "mcause",
[0x343] = "mtval",
[0x344] = "mip",
};

#define R 32

#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = SEXT(BITS(i, 31, 25) << 5 | BITS(i, 11, 7), 12); } while(0)
#define immB() do { *imm = SEXT((BITS(i, 31, 31) << 11 | BITS(i, 7, 7) << 10 | BITS(i, 30, 25) << 4 | BITS(i, 11, 8)) << 1 , 13); } while(0)
#define immJ() do { *imm = SEXT((BITS(i, 31, 31) << 19 | BITS(i, 19, 12) << 11 | BITS(i, 20, 20) << 10 | BITS(i, 30, 21)) << 1, 21); } while(0)

const char *instB[] = {
	"beq", "bne", "intrb_none", "intrb_none", "blt", "bge", "bltu", "bgeu"
};

const char *instR[] = {
	"add", "sll", "slt", "sltu", "xor", "srl", "or", "and"
};

const char *instS[] = {
	"sb", "sh", "sw"
};

const char *instI[] = {
	"addi", "slli", "slti", "sltiu", "xori", "srli", "ori", "andi"
};

const char *instLOAD[] = {
	"lb", "lh", "lw", "lbu", "lhu"
};

static void itrace_b(uint32_t func3, int rs1, int rs2, uint32_t imm) {
	if(rs1 >= R || rs2 >= R || func3 > 7) {
		printf("inst B decode fail\n");
		printf("func3 = %d, rs1 = %d, rs2 = %d, imm = %x\n", func3, rs1, rs2, imm);
		assert(0);
	}

	printf("%s %s,%s,%x\n", instB[func3], regs[rs1], regs[rs2], itrace_pc + imm);
}

static void itrace_r(uint32_t func7, uint32_t func3, int rd, int rs1, int rs2) {
	if(rd >= R || rs1 >= R || func3 > 7) {
		printf("inst R decode fail\n");
		printf("func7 = %d, func3 = %d, rd = %d, rs1 = %d, rs2 = %d\n", func7, func3, rd, rs1, rs2);
		assert(0);
	}

	if(func3 == 0) {
		if(func7 == 0) {
			printf("%s %s,%s,%s\n", instR[func3], regs[rd], regs[rs1], regs[rs2]);
		} else {
			printf("%s %s,%s,%s\n", "sub", regs[rd], regs[rs1], regs[rs2]);
		}
	} else if(func3 == 5) {
		if(func7 == 0) {
			printf("%s %s,%s,%s\n", instR[func3], regs[rd], regs[rs1], regs[rs2]);
		} else {
			printf("%s %s,%s,%s\n", "sra", regs[rd], regs[rs1], regs[rs2]);
		}
	}else {
		printf("%s %s,%s,%s\n", instR[func3], regs[rd], regs[rs1], regs[rs2]);
	}
}

static void itrace_j(uint32_t imm) {
	//printf("itrace_j: itrace_pc = 0x%08x, imm = 0x%08x\n", itrace_pc, imm);
	printf("%s %x\n", "jal", itrace_pc + imm);
}

static void itrace_s(uint32_t func3, int rs1, int rs2, uint32_t imm) {
	if(rs1 >= R || rs2 >= R || func3 > 3) {
		printf("inst S decode fail\n");
		printf("func3 = %d, rs1 = %d, rs2 = %d, imm = %x\n", func3, rs1, rs2, imm);
		assert(0);
	}
	printf("%s %s,%s,%x\n", instS[func3], regs[rs1], regs[rs2], imm);
}

static void itrace_u(uint32_t opcode, int rd, uint32_t imm) {
	if(rd >= R) {
		printf("U reg fail\n");
		printf("rd = %d, imm = %x\n", rd, imm);
		assert(0);
	}
	if(opcode == 0x17) {
		printf("%s %s,%x\n", "auipc", regs[rd], itrace_pc + imm);
	} else if(opcode == 0x37) {
		printf("%s %s,%x\n", "lui", regs[rd], imm);
	}
}

static void itrace_i(uint32_t opcode, uint32_t func7, uint32_t func3, int rd, int rs1, uint32_t imm, uint32_t shamt, uint32_t csr) {
	if(rs1 >= R || rd >= R) {
		printf("I reg fail\n");
		printf("func7 = %d, func3 = %d, rd = %d, rs1 = %d, imm = %x\n", func7, func3, rd, rs1, imm);
		assert(0);
	}
	if(opcode == 0x67) {
		printf("%s %s,%s,%x\n", "jalr", regs[rd], regs[rs1], cpu.gpr[rs1] + imm);
	} else if(opcode == 0x73) {
			switch(func3) {
				case 0x0: {
					switch(imm) {
						case 0x0: printf("ecall\n"); break;
						case 0x1: printf("ebreak\n"); break;
						case 0x105: printf("wfi\n"); break;
						case 0x302: printf("mret\n"); break;
						default:printf("inst I decode fail\n");
					}
				}; break;
				case 0x1: printf("csrrw %s,%s,%s\n", regs[rd], csrs[csr], regs[rs1]); break;
				case 0x2: printf("csrrs %s,%s,%s\n", regs[rd], csrs[csr], regs[rs1]); break;
				case 0x3: printf("csrrc %s,%s,%s\n", regs[rd], csrs[csr], regs[rs1]); break;
				case 0x5: printf("csrrwi %s,%s,%s\n", regs[rd], csrs[csr], regs[rs1]); break;
				case 0x6: printf("csrrsi %s,%s,%s\n", regs[rd], csrs[csr], regs[rs1]); break;
				case 0x7: printf("csrrci %s,%s,%s\n", regs[rd], csrs[csr], regs[rs1]); break;
				default: printf("inst I decode fail\n");
		}
	} else if(opcode == 0x13) {
		if(func3 > 7) {
			printf("inst I decode fail\n");
			printf("func7 = %d, func3 = %d, rd = %d, rs1 = %d, imm = %x\n", func7, func3, rd, rs1, imm);
			assert(0);
		}
		if(func3 == 5 && func7 == 2) {
			printf("%s %s,%s,%x\n", "srai", regs[rd], regs[rs1], shamt);
		} else {
			printf("%s %s,%s,%x\n", instI[func3], regs[rd], regs[rs1], imm);
		}
	} else if(opcode == 0x03) {
		if(func3 > 4) {
			printf("inst LOAD decode fail\n");
			printf("func7 = %d, func3 = %d, rd = %d, rs1 = %d, imm = %x\n", func7, func3, rd, rs1, imm);
			assert(0);
		}
			printf("%s %s,%x(%s)\n", instLOAD[func3], regs[rd], cpu.gpr[rs1] + imm, regs[rs1]);
	}
}

static int type_instr(uint32_t opcode) {
	if(opcode == 0x63) {
		return TYPE_B;
	} else if(opcode == 0x33) {
		return TYPE_R;
	} else if(opcode == 0x23) {
		return TYPE_S;
	} else if(opcode == 0x6f) {
		return TYPE_J;
	} else if(opcode == 0x17 || opcode == 0x37) {
		return TYPE_U;
	} else if(opcode == 0x67 || opcode == 0x73 || opcode == 0x13 || opcode == 0x03) {
		return TYPE_I;
	} else {
		return TYPE_N;
	}
}

void itrace(uint32_t instr, uint32_t pc) {
	uint32_t i = instr;
	uint32_t opcode = instr & 0x7f;
	uint32_t func3 = (instr & 0x7000) >> 12;
	uint32_t func7 = (instr & 0xfe000000) >> 25;
	uint32_t shamt = SEXT(BITS(i, 24, 20), 5);
	uint32_t csr = BITS(i, 31, 20);

	uint32_t immb = SEXT((BITS(i, 31, 31) << 11 | BITS(i, 7, 7) << 10 | BITS(i, 30, 25) << 4 | BITS(i, 11, 8)) << 1 , 13); 
	uint32_t imms = SEXT(BITS(i, 31, 25) << 5 | BITS(i, 11, 7), 12);
	uint32_t immi = SEXT(BITS(i, 31, 20), 12);
	uint32_t immu = SEXT(BITS(i, 31, 12), 20) << 12;
	uint32_t immj = SEXT((BITS(i, 31, 31) << 19 | BITS(i, 19, 12) << 11 | BITS(i, 20, 20) << 10 | BITS(i, 30, 21)) << 1, 21);

  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  int rd  = BITS(i, 11, 7);
	itrace_pc = pc;

	switch(type_instr(opcode)) {
		case TYPE_B: itrace_b(func3, rs1, rs2, immb); break;
		case TYPE_R: itrace_r(func7, func3, rd, rs1, rs2); break;
		case TYPE_S: itrace_s(func3, rs1, rs2, imms); break;
		case TYPE_J: itrace_j(immj); break;
		case TYPE_U: itrace_u(opcode, rd, immu); break;
		case TYPE_I: itrace_i(opcode, func7, func3, rd, rs1, immi, shamt, csr); break;
		default: printf("decode fail\n"); nemu_state.state = NEMU_ABORT; 
	}
}
