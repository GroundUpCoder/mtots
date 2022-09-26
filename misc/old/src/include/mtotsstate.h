#ifndef mtotsstate_h
#define mtotsstate_h

#include "mtots.h"

#include "mtotsobject.h"

struct mtots_State {
  size_t value_stack_start, value_stack_end;
  TValue value_stack[MTOTS_MAX_VALUE_STACK_SIZE];
};

#endif/*mtotsstate_h*/
