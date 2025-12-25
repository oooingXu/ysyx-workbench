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
};

#endif

static Context* do_event(Event e, Context* c) {

#ifdef STRACE
	if(e.event < 3 && c->GPR1 < 2) 
	printf("(nanos-lite): e.event = %s, c->GPR1 = %s, c->GPRx = %d\n", event_name[e.event], sys_name[c->GPR1], c->GPRx);
#endif

  switch (e.event) {
		case EVENT_YIELD: break;
		case EVENT_SYSCALL: do_syscall(c); break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
