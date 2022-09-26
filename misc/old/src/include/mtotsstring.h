#ifndef mtotsstring_h
#define mtotsstring_h

#include "mtots.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

const char *mkfmt(mtots_State *L, const char *fmt, ...);
const char *mkfmtV(mtots_State *L, const char *fmt, va_list args);

#endif/*mtotsstring_h*/
