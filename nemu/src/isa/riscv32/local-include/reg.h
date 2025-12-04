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

#ifndef __RISCV_REG_H__
#define __RISCV_REG_H__

#include <common.h>
#include <isa.h>

#define FFLAGS    0x001
#define FRM       0x002
#define MSTATUS   0x300
#define MIE  		  0x304
#define MTVEC		  0x305
#define MSCRATCH  0x340
#define MEPC		  0x341
#define MCAUSE	  0x342
#define MTVAL     0x343
#define MIP   	  0x344
#define PMPCFGR0  0x3a0
#define PMPADDR0  0x3b0
#define MVENDORID 0xf11
#define MARCHID   0xf12
#define MIMPID    0xf13
#define MHARTID   0xf14

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < MUXDEF(CONFIG_RVE, 16, 32)));
  return idx;
}

static inline uint32_t *check_csr_idx(word_t idx) {
	switch(idx) {

#ifdef CONFIG_RVDF
		case FFLAGS:		return &(cpu.fflags);
		case FRM:				return &(cpu.frm);
#endif

		case MSTATUS:   return &(cpu.mstatus);
		case MIE:       return &(cpu.mie);
		case MTVEC:     return &(cpu.mtvec);
		case MEPC:      return &(cpu.mepc);
		case MCAUSE:    return &(cpu.mcause);
		case MTVAL:     return &(cpu.mtval);
		case MSCRATCH:  return &(cpu.mscratch);
		case MIP:       return &(cpu.mip);
		case PMPCFGR0:  return &(cpu.pmpcfgr0);
		case PMPADDR0:  return &(cpu.pmpaddr0);
		case MVENDORID: return &(cpu.mvendorid);
		case MARCHID:   return &(cpu.marchid);
		case MIMPID:		return &(cpu.mimpid);
		case MHARTID:   return &(cpu.mhartid);
		default: return &cpu.gpr[0];
		//default: printf("Faild csr 0x%x\n",idx); assert(0);
	}
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)])
#define csr(idx) (*check_csr_idx(idx))

#ifdef CONFIG_RVDF
#define dgpr(idx) (cpu.dgpr[idx])
#endif

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

#endif
