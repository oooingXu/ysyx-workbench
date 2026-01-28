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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>
extern const char *regs[]; 

typedef struct {
	// Note: only a few bits are used.  (Machine = 3, User = 0)
	// Bits 0..1 = privilege.
	// Bit 2 = WFI (Wait for interrupt)
	// Bit 3+ = Load/Store reservation LSBs.
	word_t extraflags;

  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;

	word_t mepc;
	word_t mcause;
	word_t mtvec;
	word_t mstatus;

	word_t mie;
	word_t mscratch;
	word_t mtval;
	word_t mip;

	word_t wdata;

	word_t waddr;
	word_t wvalid;

	//word_t pmpcfgr0;
	//word_t pmpaddr0;

	word_t mimpid;
	word_t mhartid;

	word_t mvendorid;
	word_t marchid;

#ifdef CONFIG_RVDF
  double dgpr[32];
	word_t fflags;
	word_t frm;
#endif

	word_t cyclel;
	word_t cycleh;
	word_t timerl;
	word_t timerh;
	word_t timermatchl;
	word_t timermatchh;

	word_t trap;
	//bool INTR;

} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  union {
    uint32_t val;
  } inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
