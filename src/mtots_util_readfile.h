#ifndef mtots_util_readfile_h
#define mtots_util_readfile_h

#include "mtots_util_string.h"

/* Read the entire contents of a file, saves them to a string, and
 * returns that NULL terminated string.
 * The returned string should be freed by the caller. */
char *readFile(const char *filePath);

#endif/*mtots_util_readfile_h*/
