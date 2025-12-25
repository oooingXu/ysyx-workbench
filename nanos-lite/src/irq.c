#include <common.h>

void do_syscall(Context *c);

//#define STRACE
#ifdef STRACE
static char *event_name[] = {
"EVENT_NULL",
"EVENT_YIELD",
"EVENT_SYSCALL",
};

static char *sys_name[] = {
"SYS_exit",
"SYS_yield",
"SYS_open",
"SYS_read",
"SYS_write",
"SYS_kill",
"SYS_getpid",
"SYS_close",
"SYS_lseek",
"SYS_brk",
};

#endif

static Context* do_event(Event e, Context* c) {

#ifdef STRACE
	if(e.event < 3 && c->GPR1 < 10) 
	printf("STRACE (nanos-lite) begin: e.event = %s, c->GPR1 = %s, c->GPR1 = %d, c->GPR2 = 0x%08x, c->GPR3 = 0x%08x, c->GPR4 = 0x%08x, c->GPRx = %d\n", event_name[e.event], sys_name[c->GPR1], c->GPR1, c->GPR2, c->GPR3, c->GPR4, c->GPRx);
#endif

  switch (e.event) {
		case EVENT_YIELD: break;
		case EVENT_SYSCALL: do_syscall(c); break;
    default: panic("Unhandled event ID = %d", e.event);
  }

#ifdef STRACE
	if(e.event < 3 && c->GPR1 < 10) 
	printf("STRACE (nanos-lite) end: c->GPR1 = %d, c->GPR2 = 0x%08x, c->GPR3 = 0x%08x, c->GPR4 = 0x%08x, c->GPRx = %d\n", c->GPR1, c->GPR2, c->GPR3, c->GPR4, c->GPRx);
#endif

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
