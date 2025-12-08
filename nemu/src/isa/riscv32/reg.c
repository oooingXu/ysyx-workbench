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
#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/trace.h>
#include <memory/paddr.h>


const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display(uint32_t pc) {
	uint32_t inst = paddr_read(pc, 4);
	printf("\n(nemu) gpr:\n");
	for(int i = 0; i < 8 ; i++) {
		for(int j = 0; j < 4; j++){
			printf("%2d[%-3s] --->  0x%08x  ", j + i * 4, regs[j + i * 4], cpu.gpr[j + i * 4]);
		}
		printf("\n");
	}

	printf("\n(nemu) csr:\n");
	printf("extraflags ---> 0x%x\n",cpu.extraflags);
	printf("trap ---> 0x%x\n",cpu.trap);
	//printf("frm 		---> 0x%x\n",cpu.frm);
	printf("mepc		---> 0x%x\n",cpu.mepc);
	printf("mtvec		---> 0x%x\n",cpu.mtvec);
	printf("mcause	---> 0x%x\n",cpu.mcause);
	printf("mstatus ---> 0x%x\n",cpu.mstatus);
	printf("mtval ---> 0x%x\n",cpu.mtval);
	printf("mip ---> 0x%x\n",cpu.mip);
	printf("mie ---> 0x%x\n",cpu.mie);
	printf("timermatchl ---> 0x%x\n",cpu.timermatchl);
	printf("timermatchh ---> 0x%x\n",cpu.timermatchh);
	printf("timerl ---> 0x%x\n",cpu.timerl);
	printf("timerh ---> 0x%x\n",cpu.timerh);
	printf("cyclel ---> 0x%x\n",cpu.cyclel);
	printf("cycleh ---> 0x%x\n",cpu.cycleh);
	printf("pc = 0x%08x\n", pc);
	printf("inst = 0x%08x\n", inst);
	itrace(inst, pc);

}

word_t isa_reg_str2val(const char *s, bool *success) {
	int len = (sizeof(regs) / sizeof(regs[0]));
	const char *s1 = (s[0] == '$' ? s + 1 : s);
	int cmp = strcmp(s1,"pc");
	for(int i = 0; i < len; i++) {
		int compare = strcmp(s1,regs[i]);
		if( compare == 0){
			*success = true;
			return cpu.gpr[i];
		}
		
	}
	if( cmp == 0){
			*success = true;
			return cpu.pc;
	}
	*success = false;

  return 0;
}
