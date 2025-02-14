#include <proc.h>
#include <elf.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

/*
static uintptr_t loader(PCB *pcb, const char *filename) {
  TODO();
  return 0;
}
*/

static uintptr_t loader(PCB *pcb, const char *filename) {
  uint32_t phdr_size = sizeof(Elf_Phdr);
  Elf_Ehdr *ehdr = malloc(sizeof(Elf_Ehdr));
  Elf_Phdr *phdr = malloc(phdr_size);
  ramdisk_read((void*)ehdr, 0, sizeof(Elf_Ehdr));
  assert(*(uint32_t *)ehdr->e_ident == 0x464c457f);
//  assert(EXPECT_TYPE == ehdr->e_machine);
  uint32_t phdr_num = ehdr->e_phnum;
  for(int i = 0; i < phdr_num; i++) {
    ramdisk_read((void*)phdr, ehdr->e_phoff + i * phdr_size, phdr_size);
    if(phdr->p_type != PT_LOAD) continue;
    ramdisk_read((void*)phdr->p_vaddr, phdr->p_offset, phdr->p_filesz);
    memset((void*)phdr->p_vaddr + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
  }
  return ehdr->e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

