#ifndef __TRACE_H
#define __TRACE_H
#include <common.h>

// iringbuf
void iringbuf_add(uint32_t inst, uint32_t pc);
void iringbuf_check(void);
void iringbuf_printf(void);

// itrace
void itrace(uint32_t instr, uint32_t pc);

#endif
