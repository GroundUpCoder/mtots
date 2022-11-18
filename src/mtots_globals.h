#ifndef mtots_globals_h
#define mtots_globals_h

#include "mtots_value.h"

void defineDefaultGlobals();

ubool implStr(i16 argCount, Value *args, Value *out);
ubool implRepr(i16 argCount, Value *args, Value *out);

#endif/*mtots_globals_h*/
