#ifndef mtots_compiler_h
#define mtots_compiler_h

#include "mtots_chunk.h"
#include "mtots_object.h"

void initRules();

ObjFunction *compile(const char *source, ObjString *moduleName);
void markCompilerRoots();

#endif/*mtots_compiler_h*/
