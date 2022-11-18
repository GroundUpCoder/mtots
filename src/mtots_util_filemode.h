#ifndef mtots_util_filemode_h
#define mtots_util_filemode_h

#include "mtots_common.h"

#include <stdio.h>

typedef enum FileMode {
  FILE_MODE_READ,
  FILE_MODE_WRITE,
  FILE_MODE_APPEND,
  FILE_MODE_READ_PLUS,
  FILE_MODE_WRITE_PLUS,
  FILE_MODE_APPEND_PLUS,
  FILE_MODE_INVALID
} FileMode;

/* out must have space for at least 3 characters (including the
 * '\0' character)
 */
void fileModeToString(FileMode mode, char *out);

/* Looks at the first two characters to determine the
 * FileMode indicated by the given string.
 */
FileMode stringToFileMode(const char *modestr);

#endif/*mtots_util_filemode_h*/
