#include "mtots_util_readfile.h"

#include "mtots_util_error.h"

#include <stdio.h>
#include <stdlib.h>

char *readFile(const char *path) {
  char *buffer;
  size_t fileSize, bytesRead;
  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    panic("Could not open file \"%s\"\n", path);
  }

  /* NOTE: This might not actually be standards
   * compliant - i.e. this may fail on some platforms.
   */
  fseek(file, 0L, SEEK_END);
  fileSize = ftell(file);
  rewind(file);

  buffer = (char*)malloc(fileSize + 1);
  if (buffer == NULL) {
    panic("Not enough memory to read \"%s\"\n", path);
  }
  bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    panic("Could not read file \"%s\"\n", path);
  }
  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}
