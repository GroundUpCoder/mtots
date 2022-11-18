#include "mtots_util_error.h"

#include "mtots_util_strbuf.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void (*errorContextProvider)(StringBuffer*);
static char *errorString;

NORETURN void panic(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);
  if (errorContextProvider) {
    StringBuffer sb;
    initStringBuffer(&sb);
    errorContextProvider(&sb);
    fprintf(stderr, "%s", sb.chars);
    freeStringBuffer(&sb);
  }
  exit(1);
}

/* Potentially recoverable error. */
void runtimeError(const char *format, ...) {
  size_t len = 0;
  va_list args;
  char *ptr;
  StringBuffer sb;

  initStringBuffer(&sb);

  if (errorContextProvider) {
    errorContextProvider(&sb);
  }

  va_start(args, format);
  len += vsnprintf(NULL, 0, format, args);
  va_end(args);
  len++; /* '\n' */

  len += sb.length;

  ptr = errorString = (char*)realloc(errorString, sizeof(char) * (len + 1));

  va_start(args, format);
  ptr += vsprintf(ptr, format, args);
  va_end(args);
  ptr += sprintf(ptr, "\n");
  strcpy(ptr, sb.chars);
}

const char *getErrorString() {
  return errorString;
}

void clearErrorString() {
  free(errorString);
  errorString = NULL;
}

void setErrorContextProvider(void (*contextProvider)(StringBuffer*)) {
  errorContextProvider = contextProvider;
}

NORETURN void assertionError() {
  panic("assertion error");
}
