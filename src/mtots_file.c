#include "mtots_file.h"
#include "mtots_memory.h"

#include <string.h>

void fileModeToString(FileMode mode, char *out) {
  switch (mode) {
    case FILE_MODE_READ: strcpy(out, "r"); return;
    case FILE_MODE_WRITE: strcpy(out, "w"); return;
    case FILE_MODE_APPEND: strcpy(out, "a"); return;
    case FILE_MODE_READ_PLUS: strcpy(out, "r+"); return;
    case FILE_MODE_WRITE_PLUS: strcpy(out, "w+"); return;
    case FILE_MODE_APPEND_PLUS: strcpy(out, "a+"); return;
    case FILE_MODE_INVALID: break;
  }
  strcpy(out, "?");
}

FileMode stringToFileMode(const char *modestr) {
  if (modestr[0] != '\0' && modestr[1] == '+') {
    switch (modestr[0]) {
      case 'r': return FILE_MODE_READ;
      case 'w': return FILE_MODE_WRITE;
      case 'a': return FILE_MODE_APPEND;
    }
  }
  switch (modestr[0]) {
    case 'r': return FILE_MODE_READ;
    case 'w': return FILE_MODE_WRITE;
    case 'a': return FILE_MODE_APPEND;
  }
  return FILE_MODE_INVALID;
}
