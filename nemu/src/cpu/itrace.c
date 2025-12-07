#include <cpu/cpu.h>
#include <cpu/trace.h>
#include <isa.h>

uint32_t itrace_pc = 0;

enum {
  TYPE_I, TYPE_U, TYPE_S,TYPE_R, TYPE_J, TYPE_B, TYPE_A, TYPE_C,
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

static void itrace_a(int func_31_27, int func3, int rd, int rs1, int rs2) {
	if(rd >= R || rs1 >= R || rs2 >= R || func3 > 7) {
		printf("inst A decode fail\n");
		printf("func_31_27 = %d, func3 = %d, rs1 = %d, rs2 = %d, rd = %d\n", func_31_27, func3, rs1, rs2, rd);
		assert(0);
	}

	if(func3 == 2) {
		switch(func_31_27){
			case 0x0:  printf("%s %s,%s,%s\n", "amoadd.w",  regs[rd], regs[rs1], regs[rs2]); break;
			case 0x1:  printf("%s %s,%s,%s\n", "amoswap.w", regs[rd], regs[rs1], regs[rs2]); break;
			case 0x2:  printf("%s %s\n", "lr.w", regs[rs1]); break;
			case 0x3:  printf("%s %s,%s\n", "sc.w", regs[rs1], regs[rs2]); break;
			case 0x4:  printf("%s %s,%s,%s\n", "amoxor.w",  regs[rd], regs[rs1], regs[rs2]); break;
			case 0x8:  printf("%s %s,%s,%s\n", "amoor.w",   regs[rd], regs[rs1], regs[rs2]); break;
			case 0xc:  printf("%s %s,%s,%s\n", "amoand.w",  regs[rd], regs[rs1], regs[rs2]); break;
			case 0x10: printf("%s %s,%s,%s\n", "amomin.w",  regs[rd], regs[rs1], regs[rs2]); break;
			case 0x14: printf("%s %s,%s,%s\n", "amomax.w",  regs[rd], regs[rs1], regs[rs2]); break;
			case 0x18: printf("%s %s,%s,%s\n", "amominu.w", regs[rd], regs[rs1], regs[rs2]); break;
			case 0x1c: printf("%s %s,%s,%s\n", "amomaxu.w", regs[rd], regs[rs1], regs[rs2]); break;
			default: printf("inst A decode fail\n");
		}

	}

}

static void itrace_b(uint32_t func3, int rs1, int rs2, uint32_t imm) {
	if(rs1 >= R || rs2 >= R || func3 > 7) {
		printf("inst B decode fail\n");
		printf("func3 = %d, rs1 = %d, rs2 = %d, imm = %x\n", func3, rs1, rs2, imm);
		assert(0);
	}

	printf("%s %s,%s,%x\n", instB[func3], regs[rs1], regs[rs2], itrace_pc + imm);
}

static void itrace_c(uint32_t func3, int rs1, int rs2, uint32_t imm) {
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
	if((opcode & 3) == 3) {
		switch(opcode){
			case 0x03: return TYPE_I; break;
			case 0x13: return TYPE_I; break;
			case 0x17: return TYPE_U; break;
			case 0x23: return TYPE_S; break;
			case 0x2f: return TYPE_A; break;
			case 0x33: return TYPE_R; break;
			case 0x37: return TYPE_U; break;
			case 0x63: return TYPE_B; break;
			case 0x67: return TYPE_I; break;
			case 0x6f: return TYPE_J; break;
			case 0x73: return TYPE_I; break;
			default: return TYPE_N;
		}
	} else return TYPE_C;
}

void itrace(uint32_t instr, uint32_t pc) {
	uint32_t i = instr;
	uint32_t opcode = instr & 0x7f;
	uint32_t func3 = (instr & 0x7000) >> 12;
	uint32_t func7 = (instr & 0xfe000000) >> 25;
	uint32_t func_31_27 = (instr & 0xfe000000) >> 27;
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
		case TYPE_A: itrace_a(func_31_27, func3, rd, rs1, rs2); break;
		case TYPE_B: itrace_b(func3, rs1, rs2, immb); break;
		case TYPE_C: itrace_c(func3, rs1, rs2, immb); break;
		case TYPE_R: itrace_r(func7, func3, rd, rs1, rs2); break;
		case TYPE_S: itrace_s(func3, rs1, rs2, imms); break;
		case TYPE_J: itrace_j(immj); break;
		case TYPE_U: itrace_u(opcode, rd, immu); break;
		case TYPE_I: itrace_i(opcode, func7, func3, rd, rs1, immi, shamt, csr); break;
		default: printf("decode fail\n"); nemu_state.state = NEMU_ABORT; 
	}
}
