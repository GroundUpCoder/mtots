#ifndef mtots_util_h
#define mtots_util_h

/* Collection of self contained utilities */
/* TODO: make `panic` and `runtimeError` also self contained */

/* Utilities that would be useful in any C program */
#include "mtots_util_string.h"
#include "mtots_util_error.h"
#include "mtots_util_unicode.h"
#include "mtots_util_strbuf.h"
#include "mtots_util_escape.h"
#include "mtots_util_readfile.h"

/* Not mtots specific, but also maybe not the most generally useful utilities */
#include "mtots_util_filemode.h"

/* mtots specific */
#include "mtots_util_env.h"
#include "mtots_util_scanner.h"

#endif/*mtots_util_h*/
