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

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <difftest-def.h>
#include <cpu/difftest.h>
#include <isa.h>

#if   defined(CONFIG_PMEM_MALLOC)
static uint8_t *pmem = NULL;
#else // CONFIG_PMEM_GARRAY
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif

uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

static uint8_t psram[PSRAM_SIZE];
uint8_t* p_guest_to_host(paddr_t paddr) { return psram + paddr - PSRAM_BASE; }

static uint8_t sram[SRAM_SIZE];
static uint8_t* s_guest_to_host(paddr_t paddr) { return sram + paddr - SRAM_BASE; }

static word_t pmem_read(paddr_t addr, int len) {
	//addr = addr & 0xfffffffc;
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
	//addr = addr & 0xfffffffc;
  host_write(guest_to_host(addr), len, data);
}

static word_t psram_read(paddr_t addr, int len) {
	word_t ret = host_read(p_guest_to_host(addr), len);
	//printf("(nemu) psram_read addr = 0x%08x, data = 0x%08x, len = %d\n", addr, ret, len);
	return ret;
}

static void psram_write(paddr_t addr, int len, word_t data) {
	//printf("(nemu) psram_write pc = 0x%08x, addr = 0x%08x, data = 0x%08x, len = %d\n", cpu.pc ,addr, data, len);
	host_write(p_guest_to_host(addr), len, data);
}

static word_t sram_read(paddr_t addr, int len) {
	word_t ret = host_read(s_guest_to_host(addr), len);
	//printf("(nemu) sram_read addr = 0x%08x, data = 0x%08x, len = %d\n", addr, ret, len);
	return ret;
}

static void sram_write(paddr_t addr, int len, word_t data) {
	//printf("(nemu) sram_write pc = 0x%08x, addr = 0x%08x, data = 0x%08x, len = %d\n", cpu.pc ,addr, data, len);
	host_write(s_guest_to_host(addr), len, data);
}

static void out_of_bound(paddr_t addr) {
  //panic("(nemu) paddr: address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
  //    addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
  printf("(nemu) paddr: address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD "\n",
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
	nemu_state.state = NEMU_ABORT;
}

void init_mem() {
#if   defined(CONFIG_PMEM_MALLOC)
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif
  IFDEF(CONFIG_MEM_RANDOM, memset(pmem, rand(), CONFIG_MSIZE));
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
}

static void mem_diff_store_update(paddr_t addr, int len, word_t data) {
	mem_diff.awaddr = addr;
	mem_diff.wdata = data;
	if(len == 4) {
		mem_diff.wstrb = 0xf;
	} else if(len == 2) {
		mem_diff.wstrb = 0x3;
	} else if(len == 1) {
		mem_diff.wstrb = 0x1;
	}
}

static paddr_t mem_diff_rdata(paddr_t rdata, paddr_t addr, int len) {
	paddr_t tmp_addr = addr;
	paddr_t tmp_rdata = rdata;
	switch(tmp_addr & 0x3) {
		case 3: tmp_rdata = tmp_rdata >> 24; break;
		case 2: tmp_rdata = tmp_rdata >> 16; break;
		case 1: tmp_rdata = tmp_rdata >> 8 ; break;
		default: break;
	}

	switch(len) {
		case 4: return tmp_rdata;
		case 2: return tmp_rdata & 0xffff;
		case 1: return tmp_rdata & 0xff;
		default: return tmp_rdata;
	}

}
		
static void mem_diff_load_update(paddr_t rdata, paddr_t addr, int len) {
	mem_diff.araddr = addr;
	mem_diff.rdata  = mem_diff_rdata(rdata, addr, len);
	if(len == 4) {
		mem_diff.arsize = 2;
	} else if(len == 2) {
		mem_diff.arsize = 1;
	} else if(len == 1) {
		mem_diff.arsize = 0;
	}
}

// nolinux
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

bool in_mmio_range(paddr_t addr) {
	return ((0x10000000 <= addr) && (addr < 0x12000000));
}

static int is_eofd;

static int ReadKBByte() {
	if(is_eofd) return 0xffffffff;
	char rxchar = 0;
	int rread = read(fileno(stdin), (char *)&rxchar, 1);

	if(rread > 0) // Tricky: getchar can't be used with arrow keys
		return rxchar;
	else 
		return -1;
}

static int IsKBHit() {
	if(is_eofd) return -1;
	int byteswaiting;
	ioctl(0, FIONREAD, &byteswaiting);
	if(!byteswaiting && write(fileno(stdin), 0, 0) != 0) {
		is_eofd = 1;
		return -1;
	} // Is end-of-file for
	return !!byteswaiting;
}

static uint32_t HandleControlLoad(uint32_t addr) {
	// Emulating a 8250 / 16550 UART
	if(addr == 0x10000005) {
		return 0x60 | IsKBHit();
	} else if(addr == 0x10000000 && IsKBHit()) {
		return ReadKBByte();
	} else if(addr == 0x1100bffc) {
		return cpu.timerh;
	} else if(addr == 0x1100bff8) {
		return cpu.timerl;
	}
	return 0;
}

static void HandleControlStore(uint32_t addr, uint32_t data) {
	if(addr == 0x10000000) {
		printf("%c", data);
		fflush(stdout);
	} else if(addr == 0x11004004) { // CLINT
		cpu.timermatchh = data;
	} else if(addr == 0x11004000) { // CLINT
		cpu.timermatchl = data;
		//printf("(nemu) : timerl = 0x%08x, timermatchl = 0x%08x, mie = 0x%08x, mip = 0x%08x\n", cpu.timerl, cpu.timermatchl, cpu.mie, cpu.mip);
	} else if(addr == 0x11100000) { // SYSCON (reboot, poweroff, etc.)
		cpu.pc += 4;
	}
}

word_t paddr_read(paddr_t addr, int len) {
	//IFDEF(CONFIG_MTRACE, printf("(nemu) pread  at " FMT_PADDR " len = %d\n", addr, len));
	paddr_t rdata = 0;

  if (likely(in_pmem(addr))) {
		IFDEF(CONFIG_FMTRACE, printf("(nemu) flash READ: addr = 0x%08x, data = 0x%08x, size = %d\n", addr, pmem_read(addr, len), len));
		rdata = pmem_read(addr, len);
		mem_diff_load_update(rdata, addr, len);
		return rdata;
	} else if(in_sram(addr)) {
		IFDEF(CONFIG_SMTRACE, printf("(nemu) sram READ: addr = 0x%08x, data = 0x%08x, size = %d\n", addr, sram_read(addr, len), len));
		rdata = sram_read(addr, len);
		mem_diff_load_update(rdata, addr, len);
		return rdata;
	} else if(in_psram(addr)) {
		IFDEF(CONFIG_PMTRACE, printf("(nemu) psram READ: addr = 0x%08x, data = 0x%08x, size = %d\n", addr, psram_read(addr, len), len));
		rdata = psram_read(addr, len);
		mem_diff_load_update(rdata, addr, len);
		return rdata;
#ifdef CONFIG_NOMMULINUX
	} else if(in_mmio_range(addr)) { // UART, CLINT
		difftest_skip_ref();
		return HandleControlLoad(addr);
#endif
	} else {
		IFDEF(CONFIG_DEVICE, return mmio_read(addr, len));
		cpu.trap = 5; // Handle access violation on instruction read
	}

	if(cpu.trap == 5) 
		out_of_bound(addr);
	 //difftest_skip_ref();
	//read_unusual(addr);
  //out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
	IFDEF(CONFIG_MTRACE, printf("paddr_write at " FMT_PADDR " len = %d data = " FMT_WORD"\n" , addr, len, data));
	mem_diff_store_update(addr, len, data);

 if (likely(in_pmem(addr))) { 
	 IFDEF(CONFIG_FMTRACE, printf("(nemu) flash WRITE: addr = 0x%08x, data = 0x%08x, size = %d\n", addr, data, len));
	 pmem_write(addr, len, data); return; 
 } else if(in_sram(addr)) {
	 IFDEF(CONFIG_SMTRACE, printf("(nemu) sram WRITE: addr = 0x%08x, data = 0x%08x, size = %d\n", addr, data, len));
	 sram_write(addr, len, data); return;
 } else if(in_psram(addr)) {
	 IFDEF(CONFIG_PMTRACE, printf("(nemu) psram WRITE: addr = 0x%08x, data = 0x%08x, size = %d\n", addr, data, len));
	 psram_write(addr, len, data); return;
 } else if(in_mmio_range(addr)) { // UART, CLINT
	 difftest_skip_ref();
	 HandleControlStore(addr, data);
 } 
 else {
	 IFDEF(CONFIG_DEVICE,  mmio_write(addr, len, data); return);
	 cpu.trap = 7;
 }

 if(cpu.trap == 7)
		out_of_bound(addr);
	 //difftest_skip_ref();
}
