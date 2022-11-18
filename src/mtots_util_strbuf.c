#include "mtots_util_strbuf.h"

#include "mtots_util_error.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initStringBuffer(StringBuffer *sb) {
  sb->chars = NULL;
  sb->capacity = sb->length = 0;
}

void freeStringBuffer(StringBuffer *sb) {
  free(sb->chars);
  sb->chars = NULL;
  sb->capacity = sb->length = 0;
}

void sbputstrlen(StringBuffer *sb, const char *chars, size_t length) {
  if (sb->length + 1 + length > sb->capacity) {
    while (sb->length + length + 1 > sb->capacity) {
      sb->capacity = sb->capacity < 8 ? 8 : sb->capacity * 2;
    }
    sb->chars = realloc(sb->chars, sb->capacity);
  }
  memcpy(sb->chars + sb->length, chars, length);
  sb->length += length;
  sb->chars[sb->length] = '\0';
}

void sbputchar(StringBuffer *sb, char ch) {
  if (sb->length + 2 > sb->capacity) {
    sb->capacity = sb->capacity < 8 ? 8 : sb->capacity * 2;
    sb->chars = realloc(sb->chars, sb->capacity);
  }
  sb->chars[sb->length++] = ch;
  sb->chars[sb->length] = '\0';
}

void sbputstr(StringBuffer *sb, const char *str) {
  sbputstrlen(sb, str, strlen(str));
}

void sbprintf(StringBuffer *sb, const char *format, ...) {
  va_list args;
  int size;
  size_t usize;

  va_start(args, format);
  size = vsnprintf(NULL, 0, format, args);
  va_end(args);

  if (size < 0) {
    panic("sbprintf: encoding error");
  }

  usize = (size_t)size;

  if (sb->capacity < sb->length + usize + 1) {
    while (sb->capacity < sb->length + usize + 1) {
      sb->capacity = sb->capacity < 8 ? 8 : sb->capacity * 2;
    }
    sb->chars = realloc(sb->chars, sb->capacity);
  }

  va_start(args, format);
  vsnprintf(sb->chars + sb->length, usize + 1, format, args);
  va_end(args);

  sb->length += usize;
}

void StringBufferWriteNumber(StringBuffer *sb, double number) {
  /* Trim the trailing zeros in the number representation */
  sbprintf(sb, "%f", number);
  while (sb->length > 0 && sb->chars[sb->length - 1] == '0') {
    sb->chars[--sb->length] = '\0';
  }
  if (sb->length > 0 && sb->chars[sb->length - 1] == '.') {
    sb->chars[--sb->length] = '\0';
  }
}
