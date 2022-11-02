#ifndef mtots_panic_h
#define mtots_panic_h

#include "mtots_common.h"

/* Defined in mtots_vm.c
 * But this one function is likely to be useful everywhere,
 * even in areas where including mtots_vm.h would be heavy. */
NORETURN void panic(const char *format, ...);

#endif/*mtots_panic_h*/
