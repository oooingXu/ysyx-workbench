#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, invalid_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, invalid_write},
#include "files.h"
};

int fs_open(const char *pathname, int flags, int mode) {
	for(int fd = 0; fd < 23; ++fd) {
		if(strcmp(pathname, file_table[fd]->name)) 
			return fd;
	}
	// error
	return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {

	assert(file_table[fd].disk_offset + len <= file_table[fd].size);
	ramdisk_read(buf, file_table[fd].disk_offset, len);

	return len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
	assert(file_table[fd].disk_offset + len <= file_table[fd].size);
	ramdisk_write(buf, file_table[fd].disk_offset, len);

}

uintptr_t fd_addr = &ramdisk_start;

size_t fd_lseek(int fd, size_t offset, int whence) {
	switch(whence) {
		case SEEK_SET: fd_addr = &ramdisk_start + offset; break;
		case SEEK_CUR: fd_addr += offset; break;
		case SEEK_END: fd_addr = &ramdisk_start + file_table[fd].disk_offset + offset; break;
		default: ;
	}
	return fd_addr;
}

int fs_close(int fd) {
	return 0;
}

void init_fs() {
  // TODO: initialize the size of /dev/fb
}
