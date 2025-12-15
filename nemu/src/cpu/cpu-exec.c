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

#include <common.h>
#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>

#ifdef CONFIG_IRBTRACE
#include <cpu/trace.h>
#endif

#ifndef CONFIG_TARGET_AM
#include "monitor/sdb/sdb.h"
#endif

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 1000000

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

void device_update();

IFDEF(CONFIG_CACHESIM, FILE *pc_trace_file = NULL);

#ifdef CONFIG_CACHESIM
void cache_sim_start() {
    const char *dir_path = "/home/romeo/ysyx-workbench/npc/perf";
    const char *file_name = "pc_trace.bin";
    char file_path[256];

    // 构造完整的文件路径
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, file_name);

    // 打开文件
    pc_trace_file = fopen(file_path, "wb");
    if (!pc_trace_file) {
        perror("Failed to open pc trace file");
        exit(1);
    }
}

void cache_sim_end() {
    if (pc_trace_file) {
        fclose(pc_trace_file);
        pc_trace_file = NULL;
    }
}
#endif

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }

	// difftest
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
#ifdef CONFIG_WATCHPOINT
		WP *wp = get_head();
		while(wp != NULL){
			bool success = true;
			debug("success = %d\n",success);
			debug("expr = %s\n",wp->expr);
			uint32_t tmp = expr(wp->expr, &success);
			if(success){
				if(tmp != wp->old_value){
					wp->new_value = tmp;
					printf("old_value = %u\n",wp->old_value);
					printf("new_value = %u\n",wp->new_value);
					debug("cpu.exec.c No equal.\n");
					nemu_state.state = NEMU_STOP;
					wp->old_value = wp->new_value;
					wp->new_value = 0;
				}
			}
			else{
				printf("cpu-exec.c exper error\n");
				debug("cpu.exec.c error\n");
			}
			wp = wp->next;
		}
#endif
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg(uint32_t pc) {
  isa_reg_display(pc);
	IFDEF(CONFIG_IRBTRACE, iringbuf_printf());
  statistic();
}

static void exec_once(Decode *s, vaddr_t pc) {
	IFDEF(CONFIG_CACHESIM, if (pc_trace_file) fwrite(&pc, sizeof(pc), 1, pc_trace_file);)
	cpu.cyclel++;
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);

#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
}

static void execute(uint64_t n) {
  Decode s;
	uint32_t new_time = 0;
  for (;n > 0; n --) {
		cpu.trap = 0;
		new_time = (uint32_t)get_time();
		if(new_time <  cpu.timerl) cpu.timerh++;
			cpu.timerl = new_time;

#ifdef CONFIG_TIMER_IRQ
		// Handle Timer interrupt
		if((cpu.timerh > cpu.timermatchh || (cpu.timerh == cpu.timermatchh && cpu.timerl > cpu.timermatchl)) && (cpu.timermatchh || cpu.timermatchl)) {
			cpu.extraflags &= ~4; // Clear WFI
			cpu.mip |= 0x80; // MTIP of MIP Fire interrupt
			difftest_skip_ref();
		} else {
			cpu.mip &= ~0x80;
		}

		// If WFI, don't run processor
		if(cpu.extraflags & 4) {
			continue;
		}

		// Timer interrupt
		if((cpu.mip & (1 << 7)) && (cpu.mie & (1 << 7)) /*mtie*/ && (cpu.mstatus & 0x8) /*mie*/ && (cpu.extraflags & 0x3)) {
			if(cpu.extraflags & 0x3) cpu.trap = 0x80000007;
			else cpu.trap = 0x8000000b;

			IFDEF(CONFIG_DEBUG_TIMER_IRQ, printf("Timer interrupt: cpu.trap = 0x%08x, %d, %d, %d", cpu.trap, (cpu.mip & (1 << 7)) , (cpu.mie & (1 << 7)) /*mtie*/, (cpu.mstatus & 0x8) /*mie*/));
		} 

#endif

	  g_nr_guest_inst ++;
	  exec_once(&s, cpu.pc);

		IFDEF(CONFIG_IRBTRACE, iringbuf_add(s.isa.inst.val, cpu.pc));

		// Handle traps and interrupts
		if(cpu.trap) cpu.pc = isa_raise_intr(cpu.trap, cpu.pc);
		else cpu.pc = s.dnpc;

		if(cpu.cyclel >= 0xff000000) cpu.cycleh++;

	  trace_and_difftest(&s, cpu.pc);

	  if (nemu_state.state == NEMU_ABORT) 
			assert_fail_msg(s.pc);

	  if (nemu_state.state != NEMU_RUNNING) 
			break;

	  IFDEF(CONFIG_DEVICE, device_update());
  }
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

	IFDEF(CONGIF_RTRAVE, RingBuffer_init());

  uint64_t timer_start = get_time();

	IFDEF(CONFIG_CACHESIM, cache_sim_start());

  execute(n);

	IFDEF(CONFIG_CACHESIM, cache_sim_end());

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          cpu.pc);
      // fall through
    case NEMU_QUIT: statistic();
  }
}
