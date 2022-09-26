#include "mtots.h"
#include <stdio.h>

int main() {
  mtots_State *L = mtots_newstate();
  printf("START\n");
  mtots_panicf(L, "%% Hello %d ... (%s) %% %f", 10 + 3, "some string", 24.33);
  return 0;
}
