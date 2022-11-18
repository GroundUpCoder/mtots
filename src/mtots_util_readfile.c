#include "mtots_util_readfile.h"

#include "mtots_util_error.h"

#include <stdio.h>
#include <stdlib.h>

ubool readFileToString(const char *filePath, String **out) {
  FILE *fin = fopen(filePath, "r");
  size_t fileSize;
  char *chars;
  if (fin == NULL) {
    runtimeError("Could not open file %s", filePath);
    return UFALSE;
  }

  if (fseek(fin, 0, SEEK_END) != 0) {
    /* TODO: Support this case */
    runtimeError("SEEK_END not supported");
    return UFALSE;
  }
  fileSize = ftell(fin);
  if (fseek(fin, 0, SEEK_SET) != 0) {
    runtimeError("Could not seek to the beginning");
    return UFALSE;
  }

  chars = (char*)malloc(fileSize + 1);
  if (fread(chars, 1, fileSize, fin) != fileSize) {
    runtimeError("fread error");
    return UFALSE;
  }
  chars[fileSize] = '\0';

  *out = internString(chars, fileSize);

  return UTRUE;
}
