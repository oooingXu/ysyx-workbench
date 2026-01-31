#include <proc.h>
#include <elf.h>
#include <common.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

extern uint8_t ramdisk_start;
size_t ramdisk_read(void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {

	Log("loader info: start = 0x%08x", (uintptr_t)&ramdisk_start);

	// init Elf_Ehdr
	Elf_Ehdr *ehdr = malloc(sizeof(Elf_Ehdr));
	if(ehdr == NULL) {
		printf("Fail init ELF file\n");
	}

	// test if elf file
	if(*(uint32_t *)ehdr->e_ident == 0xBadC0de){
		printf("Not the ELF file\n");
	}

	ramdisk_read(ehdr, 0, sizeof(Elf_Ehdr));
	if(ehdr->e_shnum == 0 || ehdr->e_shentsize == 0) {
		printf("Invalid section header number of size\n");
	}

	// init Elf_Phdr
	Elf_Phdr *phdr = malloc(ehdr->e_phentsize * ehdr->e_phnum);
	if(phdr == NULL) {
		printf("Fail init Phdr\n");
	}

	ramdisk_read(phdr, ehdr->e_phoff, ehdr->e_phnum * ehdr->e_phentsize);
	
	// load program
	for(int i = 0; i < ehdr->e_phnum; ++i) {
		if(phdr[i].p_type == PT_LOAD) {
			ramdisk_read((void *)phdr[i].p_vaddr, phdr[i].p_offset, phdr[i].p_filesz);
			memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
		}
	}

	// return entry point from ELF header
  return ehdr->e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = 0x%08x", (uint32_t)entry);
  ((void(*)())entry) ();
}

