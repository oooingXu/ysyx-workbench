#include <cpu/trace.h>

#define COUNT 16

uint32_t iring_inst[COUNT];
uint32_t iring_pc[COUNT];
bool iring_check[COUNT];

int count = 0;

void count_add(int _count) {
	if(_count == 15) count = 0;
	else count++;
}	

int count_sub(int _count) {
	if(_count == 0) return 15;
	else return --_count;
}

void iringbuf_add(uint32_t inst, uint32_t pc) {
	iring_inst[count] = inst;
	iring_pc[count] = pc;
	IFDEF(CONFIG_IRINGBUF_DEBUG, printf("iringbuf_add_debug: count = %d, pc = 0x%08x, inst = 0x%08x\n", count, iring_pc[count], iring_inst[count]));
	count_add(count);
}

void iringbuf_check() {
	IFDEF(CONFIG_IRINGBUF_DEBUG, printf("iringbuf_check_debug: count = %d\n", count));
	iring_check[count_sub(count)] = true;
}

void iringbuf_abort(int num) {
	if(iring_check[num]) printf(" ----> ");
	else printf("       ");
}

void iringbuf_printf() {
	//printf("iringbuf_printf_debug: count = %d\n", count);
	for(int i = count; i < COUNT + count; i++) {
		int num = i >= COUNT ? i - COUNT : i;
		iringbuf_abort(num);
		printf("pc = 0x%08x, inst = 0x%08x, ", iring_pc[num], iring_inst[num]); 
		itrace(iring_inst[num], iring_pc[num]);
	}
}

