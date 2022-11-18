#ifndef mtots_util_string_h
#define mtots_util_string_h

#include "mtots_common.h"

typedef struct String {
  ubool isMarked;
  char *chars;
  size_t length;
  u32 hash;
} String;

String *internString(const char *chars, size_t length);
String *internCString(const char *string);
String *internOwnedString(char *chars, size_t length);
void freeUnmarkedStrings();


#endif/*mtots_util_string_h*/
