#ifndef mtots_assumptions_impl_h
#define mtots_assumptions_impl_h
#include "mtots_assumptions.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void checkAssumptions() {
  /* check type sizes */
  if (sizeof(char) != 1) { /* standard should guarantee this */
    fprintf(stderr, "char is not 1 bytes (got %lu)", (unsigned long)sizeof(char));
    exit(1);
  }
  if (sizeof(short) != 2) {
    fprintf(stderr, "short is not 2 bytes (got %lu)", (unsigned long)sizeof(short));
    exit(1);
  }
  if (sizeof(int) != 4) {
    fprintf(stderr, "int is not 4 bytes (got %lu)", (unsigned long)sizeof(int));
    exit(1);
  }
  if (sizeof(float) != 4) {
    fprintf(stderr, "float is not 4 bytes (got %lu)", (unsigned long)sizeof(float));
    exit(1);
  }
  if (sizeof(double) != 8) {
    fprintf(stderr, "double is not 8 bytes (got %lu)", (unsigned long)sizeof(double));
    exit(1);
  }
  if (sizeof(void*) != 4 && sizeof(void*) != 8) {
    fprintf(stderr, "void* is neither 4 nor 8 bytes (got %lu)", (unsigned long)sizeof(void*));
    exit(1);
  }
  if (sizeof(void*) != sizeof(size_t)) {
    fprintf(stderr, "sizeof(void*) != sizeof(size_t) (got %lu and %lu)",
      (unsigned long)sizeof(void*), (unsigned long)sizeof(size_t));
    exit(1);
  }

  /* check endianness */
  {
    unsigned int i = 0x01020304;
    unsigned char *ptr = (unsigned char*) &i;
    if (ptr[0] != 4 || ptr[1] != 3 || ptr[2] != 2 || ptr[3] != 1) {
      fprintf(stderr, "platform is not little endian\n");
      exit(1);
    }
  }

  /* check that all-bits-zero is null pointer */
  {
    int *ptr;
    memset(&ptr, 0, sizeof(ptr));
    if (ptr != NULL) {
      fprintf(stderr, "all-bits-zero is not a null pointer\n");
      exit(1);
    }
  }
}
#endif/*mtots_assumptions_impl_h*/
