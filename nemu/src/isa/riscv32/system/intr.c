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

#include <isa.h>

#define MSTATUS 0x300
#define MTVEC		0x305
#define MEPC		0x341
#define MCAUSE	0x342

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
	//if(cpu.trap & 0x80000000)  // If prefixed with 1 in MSB, int's an interrupt, not a trap.
	//	cpu.mtval = 0;
	//else 
	//	cpu.mtval = cpu.trap > 4 && cpu.trap <= 7 ? cpu.mtval : cpu.pc;
	cpu.mtval = cpu.trap > 4 && cpu.trap <= 7 ? cpu.pc : 0;

	// TRICKY: The kernel advances mepc automatically
	// mstatus & 8 = MIE, & 0x80 = MPIE
	// On an interrupt, the systemm moves current MIE into MPIE
	//cpu.mcause = cpu.trap;
	cpu.mcause = NO;
	cpu.mepc = epc;

	//printf("& 0x80 << 4 = %x, &0x8 = %x, & 0x8 << 4 = %x\n", ((cpu.mstatus & 0x80) << 4), cpu.mstatus & 0x8, ((cpu.mstatus & 0x8) << 4));
	uint32_t start_mstatus = cpu.mstatus;
	uint32_t start_extraflags = cpu.extraflags;
//	cpu.mstatus = ((cpu.mstatus & 0x80) << 4) | ((cpu.mstatus & 0x8) << 4)| ((start_extraflags & 3) << 11) ;
	//cpu.mstatus = ((cpu.mstatus & 0x8) << 4)| ((start_extraflags & 3) << 11) ;
	cpu.mstatus = ((start_extraflags & 3) << 11) ;
	cpu.extraflags = (start_mstatus >> 11) & 0x3;
	return cpu.mtvec;
	//cpu.extraflags |= 3;

  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
