#include <stdint.h>
#include "../../libs/libos/src/syscall.h"

#ifdef __ISA_NATIVE__
#error can not support ISA=native
#endif

extern int _syscall_(int, uintptr_t, uintptr_t, uintptr_t);

int main() {
  return _syscall_(SYS_yield, 0, 0, 0);
}
