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

//#include "mmu.h"
//#include "sim.h"
#include <common.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <difftest-def.h>

#define NR_GPR MUXDEF(CONFIG_RVE, 16, 32)

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
#define MHARTID   0xf14

static std::vector<std::pair<reg_t, abstract_device_t*>> difftest_plugin_devices;
static std::vector<std::string> difftest_htif_args;
static std::vector<std::pair<reg_t, mem_t*>> difftest_mem(
    1, std::make_pair(reg_t(DRAM_BASE), new mem_t(CONFIG_MSIZE)));
static debug_module_config_t difftest_dm_config = {
  .progbufsize = 2,
  .max_sba_data_width = 0,
  .require_authentication = false,
  .abstract_rti = 0,
  .support_hasel = true,
  .support_abstract_csr_access = true,
  .support_abstract_fpr_access = true,
  .support_haltgroups = true,
  .support_impebreak = true
};

struct diff_context_t {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  word_t pc;

	word_t mepc;
	word_t mcause;
	word_t mtvec;
	word_t mstatus;
	word_t mvendorid;
	word_t marchid;

	word_t mie;
	word_t mscratch;
	word_t mtval;
	word_t mip;
	word_t pmpcfgr0;
	word_t pmpaddr0;
	word_t mimpid;
	word_t mhartid;
};


static sim_t* s = NULL;
static processor_t *p = NULL;
static state_t *state = NULL;

void sim_t::diff_init(int port) {
  p = get_core("0");
  state = p->get_state();
}

void sim_t::diff_step(uint64_t n) {
  step(n);
}

void sim_t::diff_get_regs(void* diff_context) {
  struct diff_context_t* ctx = (struct diff_context_t*)diff_context;
  for (int i = 0; i < NR_GPR; i++) {
    ctx->gpr[i] = state->XPR[i];
  }
	//csr
	ctx->mstatus   = state->mstatus->read();
	ctx->mie       = state->mie->read();
	ctx->mtvec     = state->mtvec->read();
	ctx->mepc      = state->mepc->read();
	ctx->mcause    = state->mcause->read();
	ctx->mtval     = state->mtval->read();
	ctx->mip       = state->mip->read();
	ctx->pmpaddr0  = state->pmpaddr[0]->read();
	ctx->mscratch  = 0;
	ctx->pmpcfgr0  = 0;
	ctx->mvendorid = 0;
	ctx->marchid   = 0;
	ctx->mimpid		 = 0;
	ctx->mhartid   = 0;
	
	//ctx->mscratch  = state->mscratch->read();
	//ctx->pmpcfgr0  = state->pmpcfgr0->read();
	//ctx->mvendorid = state->mvendorid->read();
	//ctx->marchid   = state->marchid->read();
	//ctx->mhartid   = state->mhartid->read();

	//pc
  ctx->pc      = state->pc;
}

void sim_t::diff_set_regs(void* diff_context) {
  struct diff_context_t* ctx = (struct diff_context_t*)diff_context;
  for (int i = 0; i < NR_GPR; i++) {
    state->XPR.write(i, (sword_t)ctx->gpr[i]);
  }
  state->pc      = ctx->pc;
	//state->mstatus->write() = ctx->mstatus;
	//state->mie->write()     = ctx->mie    ;
	//state->mtvec->write()   = ctx->mtvec  ;
	//state->mepc->write()    = ctx->mepc   ;
	//state->mcause->write()  = ctx->mcause ;
	//state->mtval->write()   = ctx->mtval  ;
	//state->mip->write()     = ctx->mip    ;
	printf("(spike): diff_set_regs\n");
}

void sim_t::diff_memcpy(reg_t dest, void* src, size_t n) {
  mmu_t* mmu = p->get_mmu();
  for (size_t i = 0; i < n; i++) {
    mmu->store<uint8_t>(dest+i, *((uint8_t*)src+i));
  }
}

extern "C" {

__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  if (direction == DIFFTEST_TO_REF) {
    s->diff_memcpy(addr, buf, n);
  } else {
    assert(0);
  }
}

__EXPORT void difftest_regcpy(void* dut, bool direction) {
	if (direction == DIFFTEST_TO_REF) {
    s->diff_set_regs(dut);
  } else {
    s->diff_get_regs(dut);
  }
}

__EXPORT void difftest_exec(uint64_t n) {
  s->diff_step(n);
}

__EXPORT void difftest_init(int port) {
  difftest_htif_args.push_back("");
  const char *isa = "RV" MUXDEF(CONFIG_RV64, "64", "32") MUXDEF(CONFIG_RVE, "E", "I") "MAFDC";
  cfg_t cfg(/*default_initrd_bounds=*/std::make_pair((reg_t)0, (reg_t)0),
            /*default_bootargs=*/nullptr,
            /*default_isa=*/isa,
            /*default_priv=*/DEFAULT_PRIV,
            /*default_varch=*/DEFAULT_VARCH,
            /*default_misaligned=*/false,
            /*default_endianness*/endianness_little,
            /*default_pmpregions=*/16,
            /*default_mem_layout=*/std::vector<mem_cfg_t>(),
            /*default_hartids=*/std::vector<size_t>(1),
            /*default_real_time_clint=*/false,
            /*default_trigger_count=*/4);
  s = new sim_t(&cfg, false,
      difftest_mem, difftest_plugin_devices, difftest_htif_args,
      difftest_dm_config, nullptr, false, NULL,
      false,
      NULL,
      true);
  s->diff_init(port);
}

__EXPORT void difftest_raise_intr(uint64_t NO) {
	//printf("\n\n\ndifftest_raise_intr NO = 0x%08lx\n\n\n", NO);
  trap_t t(NO);
  p->take_trap_public(t, state->pc);
}

}
