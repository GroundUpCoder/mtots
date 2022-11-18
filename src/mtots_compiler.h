#ifndef mtots_compiler_h
#define mtots_compiler_h

#include "mtots_chunk.h"
#include "mtots_object.h"

void initParseRules();

ObjThunk *compile(const char *source, String *moduleName);
void markCompilerRoots();

#endif/*mtots_compiler_h*/
