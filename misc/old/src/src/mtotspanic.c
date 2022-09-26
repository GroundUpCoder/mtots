#include "mtots.h"
#include "mtotsstring.h"

#include <stdlib.h>
#include <stdio.h>

void mtots_panic(mtots_State *L, const char *msg) {
  fprintf(stderr, "PANIC: %s\n", msg);
  exit(1);
}

void mtots_panicf(mtots_State *L, const char *fmt, ...) {
  va_list args;
  const char *msg;
  va_start(args, fmt);
  msg = mkfmtV(L, fmt, args);
  va_end(args);
  mtots_panic(L, msg);
}
