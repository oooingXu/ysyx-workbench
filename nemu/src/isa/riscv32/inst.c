/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>

#define R(i) gpr(i)
#define C(i) csr(i)

#ifdef CONFIG_RVDF
#define D(i) dgpr(i)
#endif

#define Mr vaddr_read
#define Mw vaddr_write

#define INT32_MIN              (-2147483647-1)

enum {
  TYPE_I, TYPE_U, TYPE_S,TYPE_R, TYPE_J, TYPE_B, TYPE_M, TYPE_A, 

#ifdef CONFIG_RVC
	TYPE_CI, TYPE_CIN, TYPE_CU, TYPE_CR, TYPE_CJ, TYPE_CB, TYPE_C16SP, TYPE_C4SPN, TYPE_CLS, TYPE_CLWSP, TYPE_CSWSP,
#endif

  TYPE_N, // none
};

enum { TYPE_A_AND_B, TYPE_A_OR_B, TYPE_A_AND_NB,};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = SEXT(BITS(i, 31, 25) << 5 | BITS(i, 11, 7), 12); } while(0)
#define immB() do { *imm = SEXT((BITS(i, 31, 31) << 11 | BITS(i, 7, 7) << 10 | BITS(i, 30, 25) << 4 | BITS(i, 11, 8)) << 1 , 13); } while(0) 
#define immCB() do { *imm = SEXT(BITS(i, 12, 12) << 8 | BITS(i, 11, 10) << 3 | BITS(i, 6, 5) << 6 | BITS(i, 4, 3) << 1 | BITS(i, 2, 2) << 5, 9); } while(0) 
#define immJ() do { *imm = SEXT((BITS(i, 31, 31) << 19 | BITS(i, 19, 12) << 11 | BITS(i, 20, 20) << 10 | BITS(i, 30, 21)) << 1, 21); } while(0) 

#ifdef CONFIG_RVC
#define immCI() do { *imm = SEXT(BITS(i, 12, 12) << 5 | BITS(i, 6, 2), 6); } while(0)
#define uimmCI() do { *uimm = BITS(i, 12, 12) << 5 | BITS(i, 6, 2); } while(0)
#define immCU() do { *imm = SEXT(BITS(i, 12, 12) << 17 | BITS(i, 6, 2) << 12, 18); } while(0)
#define immC16SP() do { *imm = SEXT(BITS(i, 12, 12) << 9 | BITS(i, 6, 6) << 4 | BITS(i, 5, 5) << 6 | BITS(i, 4, 3) << 7 | BITS(i, 2, 2) << 5, 10); } while(0)
#define uimmC4SPN() do { *uimm = BITS(i, 12, 11) << 4 | BITS(i, 10, 7) << 6 | BITS(i, 6, 6) << 2 | BITS(i, 5, 5) << 3; } while(0)
#define uimmCLS() do { *uimm = BITS(i, 12, 10) << 3 | BITS(i, 6, 6) << 2 | BITS(i, 5, 5) << 6; } while(0)
#define uimmCLWSP() do { *uimm = BITS(i, 12, 12) << 5 | BITS(i, 6, 4) << 2 | BITS(i, 3, 2) << 6; } while(0)
#define uimmCSWSP() do { *uimm = BITS(i, 12, 9) << 2 | BITS(i, 8, 7) << 6; } while(0)
#define immCJ() do { *imm = SEXT((BITS(i, 12, 12) << 11 | BITS(i, 11, 11) << 4 | BITS(i, 10, 9) << 8 | BITS(i, 8, 8) << 10 | BITS(i, 7, 7) << 6| BITS(i, 6, 6) << 7 | BITS(i, 5, 3) << 1 | BITS(i, 2, 2)), 12); } while(0) 
#endif

static void csr_write(uint32_t csr, uint32_t a, uint32_t b, int op){
	uint32_t tmp = 0;
	switch(op) {
		case TYPE_A_AND_B: tmp = a & b; break;
		case TYPE_A_OR_B: tmp = a | b; break;
		case TYPE_A_AND_NB: tmp = a & (~b); break;
		default:;
	}

	if(csr == MSTATUS) tmp &= 0x807e79aa;

	C(csr) = tmp;
}

static void decode_operand(Decode *s, word_t *src1, word_t *src2, word_t *imm, word_t *uimm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  //*rd     = BITS(i, 11, 7);
	IFDEF(CONFIG_MTRACE, printf("inst = 0x%08x, rs1 = %d, rs2 = %d\n", i, rs1, rs2));
  switch (type) {
    case TYPE_I: src1R();					 immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_J:									 immJ(); break;
		case TYPE_A: case TYPE_M:
    case TYPE_R: src1R(); src2R();				 break;

#ifdef CONFIG_RVC
    case TYPE_CI:                  immCI(); break;
    case TYPE_CIN:                 uimmCI(); break;
    case TYPE_CU:                  immCU(); break;
    case TYPE_CB:                  immCB(); break;
    case TYPE_CJ:                  immCJ(); break;
    case TYPE_C16SP:               immC16SP(); break;
    case TYPE_C4SPN:               uimmC4SPN(); break;
    case TYPE_CLS:								 uimmCLS(); break;
    case TYPE_CLWSP:							 uimmCLWSP(); break;
    case TYPE_CSWSP:							 uimmCSWSP(); break;
#endif

  }
}

static int sysecall() {
	if((cpu.extraflags & 0x3) == 0) 
		return 0x8;
	else 
		return 0xb;
}

static int decode_exec(Decode *s) {
  int rd  = BITS(s->isa.inst.val, 11, 7);

#ifdef CONFIG_DEBUG_A
  int rs1 = BITS(s->isa.inst.val, 19, 15);
  int rs2 = BITS(s->isa.inst.val, 24, 20);
#endif

#ifdef CONFIG_RVC
	// C
  int rdc42    = BITS(s->isa.inst.val, 4, 2);
  int rdc97    = BITS(s->isa.inst.val, 9, 7);
  int rs1c97   = BITS(s->isa.inst.val, 9, 7);
  int rs2c42   = BITS(s->isa.inst.val, 4, 2);
  int rs1_11_7 = BITS(s->isa.inst.val, 11, 7);
  int rs2_6_2  = BITS(s->isa.inst.val, 6, 2);
#endif

  //int rm  = BITS(s->isa.inst.val, 14, 12);
	
#ifdef CONFIG_RVDF
	typedef union {
		double d;
		uint64_t u;
	} cvt;

	cvt dsrc1, dsrc2;
	dsrc1.d = D(rs1);
	dsrc2.d = D(rs2);
#endif

  word_t src1 = 0, src2 = 0, imm = 0, uimm = 0;
  s->dnpc = s->snpc;

  int zimm = BITS(s->isa.inst.val, 19, 15);
	uint32_t csr = BITS(s->isa.inst.val, 31, 20);
	//printf("inst = 0x%08x\n", s->isa.inst.val);
	//bool aq = BITS(s->isa.inst.val, 26, 26);
	//bool rl = BITS(s->isa.inst.val, 25, 25);
  //int rs2 = BITS(s->isa.inst.val, 24, 20);
#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &src1, &src2, &imm, &uimm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = ((int32_t)src1  < (int32_t)imm));
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = (src1 < (uint32_t)imm));

  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, IFDEF(CONFIG_MTRACE, printf("(nemu) lb: addr = 0x%08x, data = 0x%08x, size = %d\n", src1 + imm, (uint32_t)SEXT(Mr(src1 + imm, 1), 8) , 1)); R(rd) = SEXT(Mr(src1 + imm, 1), 8));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, IFDEF(CONFIG_MTRACE, printf("(nemu) lh: addr = 0x%08x, data = 0x%08x, size = %d\n", src1 + imm, (uint32_t)SEXT(Mr(src1 + imm, 2), 16), 2)); R(rd) = SEXT(Mr(src1 + imm, 2), 16)); 
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, IFDEF(CONFIG_MTRACE, printf("(nemu) lw: addr = 0x%08x, data = 0x%08x, size = %d\n", src1 + imm, (uint32_t)SEXT(Mr(src1 + imm, 4), 32), 4)); R(rd) = SEXT(Mr(src1 + imm, 4), 32));

  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, IFDEF(CONFIG_MTRACE, printf("(nemu) lbu: addr = 0x%08x, size = %d", src1 + imm, 1)); IFDEF(CONFIG_MTRACE, printf(" data = 0x%08x\n", (uint32_t)Mr(src1 + imm, 1))); R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, IFDEF(CONFIG_MTRACE, printf("(nemu) lhu: addr = 0x%08x, size = %d", src1 + imm, 2)); IFDEF(CONFIG_MTRACE, printf(" data = 0x%08x\n", (uint32_t)Mr(src1 + imm, 1))); R(rd) = Mr(src1 + imm, 2)); 

  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);

  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = (src1 << SEXT(imm, 5)));
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = (src1 >> SEXT(imm, 5)));
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = ((int32_t)src1  >> SEXT(imm, 5)));

  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, s->dnpc = (src1 + imm) & ~(word_t)1; IFDEF(CONFIG_FTRACE, jalr_print(s->isa.inst.val, rd, imm, s->pc, s->dnpc)); R(rd) = s->pc + 4);
	INSTPAT("??????? ????? ????? ??? ????? 11001 11", ret		 , I, s->dnpc = R(1); IFDEF(CONFIG_FTRACE, printf("0x%08x: ret  [%s]\n", s->pc, get_func_name(s->pc))));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, IFDEF(CONFIG_MTRACE, printf("(nemu) sb: addr = 0x%08x, data = 0x%08x, size = %d\n", src1 + imm, src2, 1)); Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, IFDEF(CONFIG_MTRACE, printf("(nemu) sh: addr = 0x%08x, data = 0x%08x, size = %d\n", src1 + imm, src2, 2)); Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, IFDEF(CONFIG_MTRACE, printf("(nemu) sw: addr = 0x%08x, data = 0x%08x, size = %d\n", src1 + imm, src2, 4)); Mw(src1 + imm, 4, src2));

  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, s->dnpc = (src1 == src2) ? (s->pc + imm) & ~(uint32_t)1: s->dnpc);
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, s->dnpc = (src1 != src2) ? (s->pc + imm) & ~(uint32_t)1: s->dnpc); 
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, s->dnpc = ((int32_t)src1 <   (int32_t)src2) ? (s->pc + imm) & ~(uint32_t)1: s->dnpc); 
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, s->dnpc = ((int32_t)src1 >=  (int32_t)src2) ? (s->pc + imm) & ~(uint32_t)1: s->dnpc); 
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, s->dnpc = ((uint32_t)src1 <  (uint32_t)src2) ? (s->pc + imm) & ~(uint32_t)1: s->dnpc); 
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, s->dnpc = ((uint32_t)src1 >= (uint32_t)src2) ? (s->pc + imm) & ~(uint32_t)1: s->dnpc); 

  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
	
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = (uint32_t)src1 >> SEXT(src2, 5));
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (int32_t)src1  >> SEXT(src2, 5));
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << SEXT(src2, 5));

  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2); //printf("(nemu) xor: R(%d) = %x\n", rd, R(rd)); 
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);


  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = ((int32_t)src1 < (int32_t)src2));
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = ((uint32_t)src1 < (uint32_t)src2));

  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui		 , U, R(rd) = SEXT(imm, 32));
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal		 , J, s->dnpc = s->pc + imm; IFDEF(CONFIG_FTRACE, jal_print(rd, s->pc, s->dnpc)); R(rd) = s->snpc); 

	// M
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , M, R(rd) = src1 * src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , M, R(rd) = SEXT((int32_t)src1, 32) * SEXT((int32_t)src2, 32) >> 32); 
  INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu , M, R(rd) = SEXT((int32_t)src1, 32) * (uint64_t)src2 >> 32); 
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , M, R(rd) = (uint64_t)src1 * (uint64_t)src2 >> 32); 

  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , M, R(rd) = (int32_t)src2  != 0 ? ((int32_t)src1 == INT32_MIN && (int32_t)src2 == -1) ? src1 : (int32_t)src1  / (int32_t)src2  : -1); 
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , M, R(rd) = (uint32_t)src2 != 0 ? (uint32_t)src1 / (uint32_t)src2 : -1); 

  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , M, R(rd) = (int32_t)src2  != 0 ? ((int32_t)src1 == INT32_MIN && (int32_t)src2 == -1) ? 0 : ((uint32_t)((int32_t)src1 % (int32_t)src2)): (int32_t)src1); 
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , M, R(rd) = (uint32_t)src2 != 0 ? (uint32_t)src1 % (uint32_t)src2 : (uint32_t)src1); 

#ifdef CONFIG_RVA
	// A
  INSTPAT("00000 ?? ????? ????? 010 ????? 0101111", amoadd.w,  A, uint32_t t = Mr(src1, 4); Mw(src1, 4, t + src2); R(rd) = t); 
  INSTPAT("01100 ?? ????? ????? 010 ????? 0101111", amoand.w,  A, uint32_t t = Mr(src1, 4); Mw(src1, 4, t & src2); R(rd) = t); 
  INSTPAT("10100 ?? ????? ????? 010 ????? 0101111", amomax.w,  A, uint32_t t = Mr(src1, 4); Mw(src1, 4, (int32_t)t > (int32_t)src2 ? t : src2); R(rd) = t); 
  INSTPAT("11100 ?? ????? ????? 010 ????? 0101111", amomaxu.w, A, uint32_t t = Mr(src1, 4); Mw(src1, 4, (uint32_t)t > (uint32_t)src2 ? t : src2); R(rd) = t); 
  INSTPAT("10000 ?? ????? ????? 010 ????? 0101111", amomin.w,  A, uint32_t t = Mr(src1, 4); Mw(src1, 4, (int32_t)t < (int32_t)src2 ? t : src2); R(rd) = t); 
  INSTPAT("11000 ?? ????? ????? 010 ????? 0101111", amominu.w, A, uint32_t t = Mr(src1, 4); Mw(src1, 4, (uint32_t)t < (uint32_t)src2 ? t : src2); R(rd) = t); 
  INSTPAT("01000 ?? ????? ????? 010 ????? 0101111", amoor.w,	 A, uint32_t t = Mr(src1, 4); Mw(src1, 4, t | src2); R(rd) = t); 
  INSTPAT("00100 ?? ????? ????? 010 ????? 0101111", amoxor.w,	 A, uint32_t t = Mr(src1, 4); Mw(src1, 4, t ^ src2); R(rd) = t); 
  INSTPAT("00001 ?? ????? ????? 010 ????? 0101111", amoswap.w, A, uint32_t t = Mr(src1, 4); Mw(src1, 4, src2); R(rd) = t); 
  INSTPAT("00010 ?? 00000 ????? 010 ????? 0101111", lr.w,			 A, uint32_t t = Mr(src1, 4); R(rd) = t; cpu.extraflags = (cpu.extraflags & 0x07) | (src1 << 3); IFDEF(CONFIG_DEBUG_A, printf("(nemu) lr.w: addr = 0x%08x, value = 0x%08x, rd = %d, extraflags = 0x%08x, pc = 0x%08x\n", src1, t, rd, cpu.extraflags, s->pc)));
  INSTPAT("00011 ?? ????? ????? 010 ????? 0101111", sc.w,			 A, uint32_t rval = ((cpu.extraflags >> 3) != (src1 & 0x1fffffff)); R(rd) = rval; if(!rval) { Mw(src1, 4, src2); } IFDEF(CONFIG_DEBUG_A, printf("(nemu) sc.w: R(%d) = 0x%08x, R(%d) = 0x%08x, R(%d) = 0x%08x, reserved_addr = 0x%08x, src1 = 0x%08x, rval = %d, pc = 0x%08x, dnpc = 0x%08x\n", rs1, R(rs1), rs2, R(rs2), rd, R(rd), cpu.extraflags >> 3, src1, rval, s->pc, s->dnpc))); 
#endif

#ifdef CONFIG_RVC
	// C
  INSTPAT("????? ?? ????? ???? 011 ? 00010 ????? 01", c.addi16sp,	 C16SP, R(2) = R(2) + imm; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.addi16sp: imm = %d, rdc42 = %d, pc = 0x%08x\n", imm, rdc42, s->pc))); 
  INSTPAT("????? ?? ????? ???? 000 ??????? ???? 00", c.addi4spn,	 C4SPN, R(rdc42 + 8) = R(2) + uimm; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.addi4spn: uimm = %d, rdc42 = %d, pc = 0x%08x\n", uimm, rdc42, s->pc))); 
  INSTPAT("????? ?? ????? ???? 000 ? ????? ????? 01", c.addi,			 CI, R(rd) += imm; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.addi: rd = %d, imm = %d, pc = 0x%08x\n", rd, imm, s->pc))); 

  INSTPAT("????? ?? ????? ???? 010 ? ????? ????? 01", c.li,			   CI, R(rd) = imm; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.li: rd = %d, imm = %d, pc = 0x%08x\n", rd, imm, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100 ? 10 ??? ????? 01", c.andi,		 CI, R(8 + rdc97) &= imm; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.andi: rdc97 = %d, imm = %d, pc = 0x%08x\n", rdc97, imm, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100 ? 01 ??? ????? 01", c.srai,		 CIN, R(8 + rdc97) = (int32_t)R(8 + rdc97) >> uimm; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.srai: rdc97 = %d, uimm = %d, pc = 0x%08x\n", rdc97, uimm, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100 ? 00 ??? ????? 01", c.srli,		 CIN, R(8 + rdc97) = R(8 + rdc97) >> uimm; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.srai: rdc97 = %d, uimm = %d, pc = 0x%08x\n", rdc97, uimm, s->pc))); 
  INSTPAT("????? ?? ????? ???? 000 ? ????? ????? 10", c.slli,			 CIN, R(rd) = R(rd) << uimm; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.slli: rd = %d, uimm = %d, pc = 0x%08x\n", rd, uimm, s->pc))); 

  INSTPAT("????? ?? ????? ???? 011 ? ????? ????? 01", c.lui,			 CU, R(rd) = imm; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.lui: rd = %d, imm = 0x%08x, pc = 0x%08x\n", rd, imm, s->pc))); 
	
  INSTPAT("????? ?? ????? ???? 100 1 00000 00000 10", c.ebreak,		 N, NEMUTRAP(s->pc, R(10)); IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.ebreak: pc = 0x%08x\n", s->pc))); 

  INSTPAT("????? ?? ????? ???? 101 ??????????? 01", c.j,					 CJ, s->dnpc = s->pc + imm; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.j: dnpc = 0x%08x, pc = 0x%08x\n", s->dnpc, s->pc))); 
  INSTPAT("????? ?? ????? ???? 001 ??????????? 01", c.jal,				 CJ, R(1) = s->pc + 2; s->dnpc = s->pc + imm; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.jal: dnpc = 0x%08x, pc = 0x%08x\n", s->dnpc, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100 1 ????? 00000 10", c.jalr,			 CJ, uint32_t t = s->pc + 2; s->dnpc = R(rs1_11_7); R(1) = t; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.jalr: rs1_11_7 = %d, dnpc = 0x%08x, pc = 0x%08x\n", rs1_11_7, s->dnpc, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100 0 ????? 00000 10", c.jr,				 CJ, s->dnpc = R(rs1_11_7); IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.jr: rs1_11_7 = %d, dnpc = 0x%08x\n", rs1_11_7, s->dnpc))); 


  INSTPAT("????? ?? ????? ???? 100011 ??? 11 ??? 01", c.and,			 CR, R(8 + rdc97) &= R(8 + rs2c42); s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.and: rdc97 = %d, rs2c42 = %d, pc = 0x%08x\n", rdc97, rs2c42, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100 1 ????? ????? 10", c.add,			 CR, R(rd) += R(rs2_6_2); s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.add: rd = %d, rs2_6_2 = %d, pc = 0x%08x\n", rd, rs2_6_2, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100011 ??? 00 ??? 01", c.sub,			 CR, R(8 + rdc97) -= R(8 + rs2c42); s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.sub: rdc97 = %d, rs2c42 = %d, pc = 0x%08x\n", rdc97, rs2c42, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100011 ??? 01 ??? 01", c.xor,			 CR, R(8 + rdc97) ^= R(8 + rs2c42); s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.xor: rdc97 = %d, rs2c42 = %d, pc = 0x%08x\n", rdc97, rs2c42, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100011 ??? 10 ??? 01", c.or,				 CR, R(8 + rdc97) |= R(8 + rs2c42); s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.or: rdc97 = %d, rs2c42 = %d, pc = 0x%08x\n", rdc97, rs2c42, s->pc))); 
  INSTPAT("????? ?? ????? ???? 100 0 ????? ????? 10", c.mv,				 CR, R(rd) = R(rs2_6_2); s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.mv: rd = %d, rs2_6_2 = %d, pc = 0x%08x\n", rd, rs2_6_2, s->pc))); 

  INSTPAT("????? ?? ????? ???? 110 ??? ??? ????? 01", c.beqz,			 CB, if(R(8 + rs1c97) == 0) s->dnpc = s->pc + imm; else s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.beqz: rs1c97 = %d, imm = 0x%08x, dnpc = 0x%08x, pc = 0x%08x\n", rs1c97, imm, s->dnpc, s->pc))); 
  INSTPAT("????? ?? ????? ???? 111 ??? ??? ????? 01", c.bnez,			 CB, if(R(8 + rs1c97) != 0) s->dnpc = s->pc + imm; else s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.beqz: rs1c97 = %d, imm = 0x%08x, dnpc = 0x%08x, pc = 0x%08x\n", rs1c97, imm, s->dnpc, s->pc))); 

  INSTPAT("????? ?? ????? ???? 010 ??? ??? ?? ??? 00", c.lw,			 CLS, uint32_t t = Mr(R(8 +rs1c97) + uimm, 4); R(rdc42 + 8) = t; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.lw: rdc42 = %d, rs1c97 = %d, uimm = %d, mem = 0x%08x, pc = 0x%08x\n", rdc42, rs1c97, uimm, t, s->pc))); 
  INSTPAT("????? ?? ????? ???? 110 ??? ??? ?? ??? 00", c.sw,			 CLS, Mw(R(8 + rs1c97) + uimm, 4, R(8 + rs2c42)); s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.sw: rdc42 = %d, rs1c97 = %d, rs2c42 = %d, uimm = %d, mem = 0x%08x, pc = 0x%08x\n", rdc42, rs1c97, rs2c42, uimm, R(8 + rs2c42), s->pc))); 
  INSTPAT("????? ?? ????? ???? 010 ? ????? ????? 10", c.lwsp,			 CLWSP, uint32_t t = Mr(R(2) + uimm, 4); R(rd) = t; s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.lwsp: rd = %d, uimm = %d, mem = 0x%08x, pc = 0x%08x\n", rd, uimm, t, s->pc))); 
  INSTPAT("????? ?? ????? ???? 110 ?????? ????? 10", c.swsp,		 CSWSP, Mw(R(2) + uimm, 4, R(rs2_6_2)); s->dnpc = s->pc + 2; IFDEF(CONFIG_DEBUG_C, printf("(nemu) : Doing c.swsp: rs2_6_2 = %d, uimm = %d, mem = 0x%08x, pc = 0x%08x\n", rs2_6_2, uimm, R(rs2_6_2), s->pc))); 
#endif


#ifdef CONFIG_RVDF
	// D
	INSTPAT("????? ?? ????? ????? 011 ????? 0000111", fld,			 I, D(rd) = ((uint64_t)Mr(R(rs1) + imm + 4, 4) << 32) | Mr(R(rs1) + imm, 4));
	INSTPAT("????? ?? ????? ????? 011 ????? 0100111", fsd,			 S, Mw(R(rs1) + imm + 4, 4, dsrc2.u >> 32); Mw(R(rs1) + imm, 4, (uint32_t)dsrc2.u));
	INSTPAT("00000 01 ????? ????? ??? ????? 1010011", fadd.d,		 R, D(rd) = dsrc1.d + dsrc2.d);
	INSTPAT("00001 01 ????? ????? ??? ????? 1010011", fsub.d,		 R, D(rd) = dsrc1.d - dsrc2.d);
	INSTPAT("00010 01 ????? ????? ??? ????? 1010011", fmul.d,		 R, D(rd) = dsrc1.d * dsrc2.d);
	INSTPAT("00011 01 ????? ????? ??? ????? 1010011", fdiv.d,		 R, D(rd) = dsrc1.d / dsrc2.d);
	INSTPAT("10100 01 ????? ????? 010 ????? 1010011", feq.d,		 R, R(rd) = dsrc1.d == dsrc2.d);
	INSTPAT("10100 01 ????? ????? 001 ????? 1010011", flt.d,		 R, R(rd) = dsrc1.d <  dsrc2.d);
	INSTPAT("10100 01 ????? ????? 000 ????? 1010011", fle.d,		 R, R(rd) = dsrc1.d <= dsrc2.d);
	INSTPAT("00100 01 ????? ????? 000 ????? 1010011", fsgnj.d,	 R, D(rd) = (dsrc1.u & 0x7fffffffffffffff) | (dsrc2.u & 0x1000000000000000));
	INSTPAT("11000 01 00000 ????? ??? ????? 1010011", fcvt.w.d,  R, R(rd) = (int32_t)dsrc1.d);
	INSTPAT("11010 01 00000 ????? ??? ????? 1010011", fcvt.d.w,  R, D(rd) = (double)(int32_t)src1);
	INSTPAT("11010 01 00001 ????? ??? ????? 1010011", fcvt.d.wu, R, D(rd) = (double)(uint32_t)src1);
#endif

	// csr
	INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw	 , I, uint32_t t = C(csr); csr_write(csr, 0, src1, TYPE_A_OR_B);   R(rd) = t; IFDEF(CONFIG_ETRACE, printf("(nemu) csrrw: pc = 0x%08x, t = 0%08x, src1 = 0x%08x, C(%x) = 0x%08x, R(%d) = 0x%08x\n",  cpu.pc, t, src1, csr, C(csr), rd, R(rd)))); // rtt need
	INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs	 , I, uint32_t t = C(csr); csr_write(csr, t, src1, TYPE_A_OR_B);   R(rd) = t; IFDEF(CONFIG_ETRACE, printf("(nemu) csrrs: pc = 0x%08x, t = 0%08x, src1 = 0x%08x, C(%x) = 0x%08x, R(%d) = 0x%08x\n",  cpu.pc, t, src1, csr, C(csr), rd, R(rd)))); // rtt need
	INSTPAT("??????? ????? ????? 011 ????? 11100 11", csrrc	 , I, uint32_t t = C(csr); csr_write(csr, t, src1, TYPE_A_AND_NB); R(rd) = t; IFDEF(CONFIG_ETRACE, printf("(nemu) csrrc: pc = 0x%08x, t = 0%08x, src1 = 0x%08x, C(%x) = 0x%08x, R(%d) = 0x%08x\n",  cpu.pc, t, src1, csr, C(csr), rd, R(rd))));
	INSTPAT("??????? ????? ????? 101 ????? 11100 11", csrrwi , I, uint32_t t = C(csr); csr_write(csr, 0, zimm, TYPE_A_OR_B);   R(rd) = t; IFDEF(CONFIG_ETRACE, printf("(nemu) csrrwi: pc = 0x%08x, t = 0%08x, src1 = 0x%08x, C(%x) = 0x%08x, R(%d) = 0x%08x\n", cpu.pc, t, src1, csr, C(csr), rd, R(rd))));
	INSTPAT("??????? ????? ????? 110 ????? 11100 11", csrrsi , I, uint32_t t = C(csr); csr_write(csr, t, zimm, TYPE_A_OR_B);   R(rd) = t; IFDEF(CONFIG_ETRACE, printf("(nemu) csrrsi: pc = 0x%08x, t = 0%08x, src1 = 0x%08x, C(%x) = 0x%08x, R(%d) = 0x%08x\n", cpu.pc, t, src1, csr, C(csr), rd, R(rd))));
	INSTPAT("??????? ????? ????? 111 ????? 11100 11", csrrci , I, uint32_t t = C(csr); csr_write(csr, t, zimm, TYPE_A_AND_NB); R(rd) = t; IFDEF(CONFIG_ETRACE, printf("(nemu) csrrci: pc = 0x%08x, t = 0%08x, src1 = 0x%08x, C(%x) = 0x%08x, R(%d) = 0x%08x\n", cpu.pc, t, src1, csr, C(csr), rd, R(rd))));

	// system
	INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall  , N, IFDEF(CONFIG_ETRACE, printf("ecall: dnpc = 0x%08x\n", cpu.mtvec)); s->dnpc = isa_raise_intr(sysecall(), s->pc));
	INSTPAT("0011000 00010 00000 000 00000 11100 11", mret   , N, IFDEF(CONFIG_ETRACE, printf("met: dnpc = 0x%08x\n", s->dnpc)); 
			uint32_t startmstatus = cpu.mstatus; 
			uint32_t startextraflags = cpu.extraflags; 
			// to clean MPRV  and move mstatus.MPIE to mstatus.MIE and set mstatus.MPIE = 1 and set mstatus.MPP = 0
			cpu.mstatus = (startmstatus & 0x807c61a2) | (startmstatus & 0x80) >> 4 | 0x80; 
			// set priv = mstatus.MPP
			cpu.extraflags = (startextraflags & ~3) | ((startmstatus >> 11) & 3); 
			IFDEF(CONFIG_ETRACE, printf("(nemu) mret: timerl = 0x%08x, timermatchl = 0x%08x, mie = 0x%08x, mip = 0x%08x, mstatus.mpie = 0x%08x, mstatus.mie = 0x%08x, startmstatus = 0x%08x\n", cpu.timerl, cpu.timermatchl, cpu.mie, cpu.mip, cpu.mstatus & 0x80, cpu.mstatus & 0x8, startmstatus));
			s->dnpc = cpu.mepc);
#ifdef CONFIG_NOMMULINUX
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, cpu.trap = 3); // R(10) is $a0
#else
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
#endif
  INSTPAT("0001000 00101 00000 000 00000 11100 11", wfi		 , N, cpu.extraflags |= 4; cpu.mstatus |= 8);
	INSTPAT("0000000 00000 00000 001 00000 0001111", fence.i, I, );
	INSTPAT("0000 ???? ???? 00000 000 00000 0001111", fence	 , R, );


  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0
	//printf("(nemu) R(rd) = %x, src1 = %x, src2 = %x, imm = %x\n", R(rd), src1, src2, imm);

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
