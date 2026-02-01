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

#ifdef CONFIG_RVE
#define R 16
#else 
#define R 32
#endif

MEM_DIFF mem_diff;

#ifdef CONFIG_PDIFF
static void cpu_info() {
	printf("ref info\n");
	for(int i = 0; i < R / 4; i++) {
		for(int j = 0; j < 4; j++) {
			printf("gpr[%d] = %08x\t", i * 4 + j, cpu.gpr[i * 4 + j]);
		}
		printf("\n");
	}
	printf("cpu.privilege = 0x%08x\n", cpu.extraflags & 3);
	printf("cpu.pc        = 0x%08x\n", cpu.pc);
	printf("cpu.mepc      = 0x%08x\n", cpu.mepc);
	printf("cpu.mcause    = 0x%08x\n", cpu.mcause);
	printf("cpu.mtvec     = 0x%08x\n", cpu.mtvec);
	printf("cpu.mstatus   = 0x%08x\n", cpu.mstatus);
	printf("cpu.mvendorid = 0x%08x\n", cpu.mvendorid);
	printf("cpu.marchid   = 0x%08x\n", cpu.marchid);
	printf("cpu.inst      = 0x%08x\n", mem_diff.inst);
	printf("cpu.wstrb     = 0x%08x\n", mem_diff.wstrb);
}
#endif

__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, int direction) {
	if(addr != 0){
		if( direction == DIFFTEST_TO_REF) {
			IFDEF(CONFIG_PDIFF, printf("REF: difftest_memcpy: addr = 0x%08x\n", addr));
			memcpy(guest_to_host(addr), buf, n);
		} else if(direction == DIFFTEST_TO_NPC) {
			IFDEF(CONFIG_PDIFF, printf("NPC: difftest_memcpy: addr = 0x%08x\n", addr));
			memcpy(guest_to_host(addr), buf, n);
			//memcpy(p_guest_to_host(addr), buf, n);
		} else {
			assert(0);
		}
	}
}

__EXPORT void difftest_regcpy(void *dut, bool direction) {
	CPU_state *diff_dut = (CPU_state *)dut;
	if(direction == DIFFTEST_TO_REF){
			memcpy(&cpu.gpr ,diff_dut->gpr, R * sizeof(cpu.gpr[0]));
			cpu.extraflags = diff_dut->extraflags;
			cpu.pc         = diff_dut->pc;
			cpu.mepc       = diff_dut->mepc;
			cpu.mcause     = diff_dut->mcause;
			cpu.mtvec      = diff_dut->mtvec;
			cpu.mstatus    = diff_dut->mstatus;
			//cpu.mvendorid  = diff_dut->mvendorid;
			//cpu.marchid    = diff_dut->marchid;
			IFDEF(CONFIG_PDIFF, printf("(nemu) difftest_regcpu: TO REF\n"));
	} else {
		memcpy(diff_dut->gpr, &cpu.gpr, R * sizeof(cpu.gpr[0]));
		diff_dut->extraflags = cpu.extraflags;
		diff_dut->pc         = cpu.pc;
		diff_dut->mepc       = cpu.mepc;
		diff_dut->mcause     = cpu.mcause;
		diff_dut->mtvec      = cpu.mtvec;
		diff_dut->mstatus    = cpu.mstatus;
		//diff_dut->mvendorid  = cpu.mvendorid;
		//diff_dut->marchid    = cpu.marchid;
		IFDEF(CONFIG_PDIFF, printf("(nemu) difftest_regcpu: TO DUT\n"));
	}
	IFDEF(CONFIG_PDIFF, cpu_info());
}

__EXPORT void difftest_exec(uint64_t n) {
	cpu_exec(n);
#ifdef CONFIG_PDIFF
	printf("(nemu) difftest_exec\n");
	cpu_info();
#endif
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

__EXPORT void difftest_mem_diff(void *dut) {
	MEM_DIFF *diff_dut = (MEM_DIFF *)dut;

	diff_dut->inst		 = mem_diff.inst;
	diff_dut->araddr   = mem_diff.araddr;
	diff_dut->awaddr   = mem_diff.awaddr;
	diff_dut->wdata    = mem_diff.wdata;
	diff_dut->rdata    = mem_diff.rdata;
	diff_dut->wstrb    = mem_diff.wstrb;
	diff_dut->arsize   = mem_diff.arsize;
}

__EXPORT void difftest_xmem(uint32_t addr) {
	printf("(nemu): data = 0x%08x\n", paddr_read(addr, 4));
}

__EXPORT void difftest_preg(uint32_t addr) {
	isa_reg_display(cpu.pc);
}

