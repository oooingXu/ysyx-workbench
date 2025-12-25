#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

#define TIMERMATCHH 0x11004004
#define TIMERMATCHL 0x11004000

static Context* (*user_handler)(Event, Context*) = NULL;

void set_timermatch() {
	*(volatile uint32_t *)TIMERMATCHL += 1000;
}

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};

    switch (c->mcause) {
			// ecall
			case 0x8: case 0x9: case 0xb: 
				// judge if SYSCALL from a5/a7
				//printf("(nemu am_cte): c->GPR1 = %d\n", c->GPR1);
				switch(c->GPR1) {
					case -1: ev.event = EVENT_YIELD; break;
					default: ev.event =  EVENT_SYSCALL; 
				}

				c->mepc += 4; break;
			// timer irq
			case 0x80000007: case 0x80000009: case 0x8000000b:
				ev.event = EVENT_IRQ_TIMER; set_timermatch(); c->mepc += 4; break;
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

	// set timermatchl
	*(volatile uint32_t *)TIMERMATCHL += 1000;

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
	Context *cp = (Context *)(kstack.end - sizeof(Context));
	cp->mepc = (uintptr_t)entry;
	cp->mstatus = 0x1880;
	cp->gpr[10] = (uintptr_t)(arg);
	
	return cp;
 // return NULL;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
	 uint32_t mie_val;
	 uint32_t mstatus_val;
   asm volatile("csrr %0, mie" : "=r"(mie_val));  // 读取当前 mie 值
   asm volatile("csrr %0, mstatus" : "=r"(mstatus_val));  // 读取当前 mstatus 值

   if (enable) {
		mie_val |= 0x80;      // 设置时钟中断位 (MTIE)
		mstatus_val |= 0x8;   // 设置时钟中断位 (mstatus.MIE)
	 } else {
		mie_val &= ~0x80;     // 清除时钟中断位
		mstatus_val &= ~0x8;  // 清除时钟中断位
	 }

   asm volatile("csrw mie, %0" : : "r"(mie_val));  // 写回 mie
   asm volatile("csrw mstatus, %0" : : "r"(mstatus_val));  // 写回 mstatus
}
