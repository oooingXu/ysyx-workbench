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

#include <cpu/difftest.h>
#include <memory/paddr.h>
#include <common.h>
#include <cpu/trace.h>
#include "../local-include/reg.h"

static uint32_t ref_mstatus, dut_mstatus;

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
	bool check = true;
	uint32_t inst = paddr_read(pc, 4);

#ifdef CONFIG_DEBUG_DIFFTEST
	printf("\n(nemu) gpr\n");
	printf("pc = 0x%08x\n", pc);
	printf("dnpc = 0x%08x\n", cpu.pc);
	printf("inst = 0x%08x\n", inst);
	itrace(inst, pc);
	for(int i = 0; i < 32 / 4 ; i++) {
		for(int j = 0; j < 4; j++){
			printf("%2d[%-3s] --->  0x%08x  ", j + i * 4, regs[j + i * 4], cpu.gpr[j + i * 4]);
		}
		printf("\n");
	}
#endif

	if(ref_r->pc != cpu.pc) {
		printf("ref_r->dnpc = 0x%08x, cpu.dnpc = 0x%08x\n", ref_r->pc, cpu.pc);
		check = false;
	}
	for(int i = 0; i < 32; i++){
		if(ref_r->gpr[i] != cpu.gpr[i]){
			printf("ref_r->gpr[%d] = 0x%08x, cpu.gpr[%d] = 0x%08x, pc = 0x%08x, inst = 0x%08x\n", i, ref_r->gpr[i], i, cpu.gpr[i], pc, inst);
			check = false;
		}
	}

	if(ref_r->mtvec != cpu.mtvec) {
		printf("ref_r->mtvec = 0x%08x, cpu.mtvec = 0x%08x\n", ref_r->mtvec, cpu.mtvec);
		check = false;
	}

	if(ref_r->mcause != cpu.mcause) {
		printf("ref_r->mcause = 0x%08x, cpu.mcause = 0x%08x\n", ref_r->mcause, cpu.mcause);
		check = false;
	}

	if(ref_r->mepc != cpu.mepc) {
		printf("ref_r->mepc = 0x%08x, cpu.mepc = 0x%08x\n", ref_r->mepc, cpu.mepc);
		check = false;
	}

	if(ref_r->mstatus != cpu.mstatus) {
		printf("last: ref_r->mstatus = 0x%08x, cpu.mstatus = 0x%08x\n", ref_mstatus, dut_mstatus);
		printf("this: ref_r->mstatus = 0x%08x, cpu.mstatus = 0x%08x\n", ref_r->mstatus, cpu.mstatus);
		check = false;
	}

	ref_mstatus = ref_r->mstatus;
	dut_mstatus = cpu.mstatus;

	if(ref_r->mie != cpu.mie) {
		printf("ref_r->mie = 0x%08x, cpu.mie = 0x%08x\n", ref_r->mie, cpu.mie);
		check = false;
	}

	if(ref_r->mip != cpu.mip) {
		printf("ref_r->mip = 0x%08x, cpu.mip = 0x%08x\n", ref_r->mip, cpu.mip);
		check = false;
	}

	if(ref_r->mtval != cpu.mtval) {
		printf("ref_r->mtval = 0x%08x, cpu.mtval = 0x%08x\n", ref_r->mtval, cpu.mtval);
		check = false;
	}

	//if(ref_r->pmpaddr0 != cpu.pmpaddr0) {
	//	printf("ref_r->pmpaddr0 = 0x%08x, cpu.pmpaddr0 = 0x%08x\n", ref_r->pmpaddr0, cpu.pmpaddr0);
	//	check = false;
	//}

	//if(ref_r->mscratch != cpu.mscratch) {
	//	printf("ref_r->mscratch = 0x%08x, cpu.mscratch = 0x%08x\n", ref_r->mscratch, cpu.mscratch);
	//	check = false;
	//}
	//
	//if(ref_r->pmpcfgr0 != cpu.pmpcfgr0) {
	//	printf("ref_r->pmpcfgr0 = 0x%08x, cpu.pmpcfgr0 = 0x%08x\n", ref_r->pmpcfgr0, cpu.pmpcfgr0);
	//	check = false;
	//}
	//
	//if(ref_r->marchid != cpu.marchid) {
	//	printf("ref_r->marchid = 0x%08x, cpu.marchid = 0x%08x\n", ref_r->marchid, cpu.marchid);
	//	check = false;
	//}

	//if(ref_r->mhartid != cpu.mhartid) {
	//	printf("ref_r->mhartid = 0x%08x, cpu.mhartid = 0x%08x\n", ref_r->mhartid, cpu.mhartid);
	//	check = false;
	//}

	//if(ref_r->mvendorid != cpu.mvendorid) {
	//	printf("ref_r->mvendorid = 0x%08x, cpu.mvendorid = 0x%08x\n", ref_r->mvendorid, cpu.mvendorid);
	//	check = false;
	//}

  return check;
}

void isa_difftest_attach() {
}
