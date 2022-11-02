#ifndef mtots_stack_h
#define mtots_stack_h

#include "mtots_value.h"

/* Stack operations.
 * Defined in mtots_vm.c */
void push(Value value);
Value pop();

#endif/*mtots_stack_h*/
