#include <cpu.h>

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
	printf("(npc) gprs\n");
	for(int i = 0; i < R / 4; i++) {
		for(int j = 0; j < 4; j++){
			printf("%-3s --->  0x%08x ",regs[j + i * 4],cpu.gpr[j + i * 4]);
		}
		printf("\n");
	}
	printf("(npc) csrs\n");
	printf("mepc = 0x%08x, ", cpu.mepc);
	printf("mstatus = 0x%08x, ", cpu.mstatus);
	printf("mcause = 0x%08x, ", cpu.mcause);
	printf("mtvec = 0x%08x\n", cpu.mtvec);
	printf("\n");
}

uint32_t isa_reg_str2val(const char *s, bool *success) {
	const char *s1 = (s[0] == '$' ? s + 1 : s);
	int cmp = strcmp(s1,"pc");
	for(int i = 0; i < R; i++) {
		int compare = strcmp(s1,regs[i]);
		if( compare == 0){
			*success = true;
			return cpu.gpr[i];
		}
		
	}
	if(cmp == 0){
			*success = true;
			return cpu.pc;
	}
	*success = false;

  return 0;
}



