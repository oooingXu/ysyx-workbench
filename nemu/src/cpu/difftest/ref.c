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
#include <cpu/cpu.h>
#include <difftest-def.h>
#include <memory/paddr.h>

#define R 16

void cpu_info() {
	printf("(nemu) regs\n");
	for(int i = 0; i < R / 4; i++){
		for(int j = 0; j < 4; j++){
			printf("gpr[%2d] = 0x%08x ", j + i * 4, cpu.gpr[j + i * 4]);
		}
		printf("\n");
	}
	printf("(nemu) csrs\n");
	printf("mepc = 0x%08x mstatus = 0x%08x mcause = 0x%08x mtvec = 0x%08x\n", cpu.mepc, cpu.mstatus, cpu.mcause, cpu.mtvec);
	printf("mvendorid = 0x%08x marchid = 0x%08x\n", cpu.mvendorid, cpu.marchid);
	printf("\n");
}

__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, int direction) {
	if(addr != 0){
		if( direction == DIFFTEST_TO_REF) {
#ifdef CONFIG_CPUINFO
			printf("DFFTEST_TO_REF\n");
			printf("difftest_memcpy: addr = 0x%08x, inst = 0x%08x\n", addr, *(uint32_t *)buf);
#endif
			memcpy(guest_to_host(addr), buf, n);
		} else if(direction == DIFFTEST_TO_NPC) {
#ifdef CONFIG_CPUINFO
			printf("DFFTEST_TO_NPC\n");
			printf("difftest_memcpy: addr = 0x%08x, inst = 0x%08x\n", addr, *(uint32_t *)buf);
#endif
			memcpy(p_guest_to_host(addr), buf, n);
		} else {
			assert(0);
		}
	}
#ifdef CONFIG_CPUINFO
	printf("difftest_memcpy\n");
	cpu_info();
#endif
}

__EXPORT void difftest_regcpy(void *dut, bool direction) {
	CPU_state *diff_dut = (CPU_state *)dut;
	if(direction == DIFFTEST_TO_REF){
			memcpy(&cpu.gpr ,diff_dut->gpr, R * sizeof(cpu.gpr[0]));
			cpu.pc        = diff_dut->pc;
			cpu.mepc      = diff_dut->mepc;
			cpu.mcause    = diff_dut->mcause;
			cpu.mtvec     = diff_dut->mtvec;
			cpu.mstatus   = diff_dut->mstatus;
			cpu.mvendorid = diff_dut->mvendorid;
			cpu.marchid   = diff_dut->marchid;
	} 
	else {
		memcpy(diff_dut->gpr, &cpu.gpr, R * sizeof(cpu.gpr[0]));
		diff_dut->pc        = cpu.pc;
		diff_dut->mepc      = cpu.mepc;
		diff_dut->mcause    = cpu.mcause;
		diff_dut->mtvec     = cpu.mtvec;
		diff_dut->mstatus   = cpu.mstatus;
		diff_dut->mvendorid = cpu.mvendorid;
		diff_dut->marchid   = cpu.marchid;
	}
#ifdef CONFIG_CPUINFO
	printf("difftest_regcpy\n");
	cpu_info();
#endif
}

__EXPORT void difftest_exec(uint64_t n) {
	cpu_exec(n);
}

__EXPORT void difftest_raise_intr(word_t NO) {
  assert(0);
}

__EXPORT void difftest_init(int port) {
  void init_mem();
  init_mem();
  /* Perform ISA dependent initialization. */
  init_isa();
}
