#ifndef mtots_util_error_h
#define mtots_util_error_h

#include "mtots_util_strbuf.h"

/* Flag a fatal error. The message is written to stderr and the program exits
 * with non-zero return code */
NORETURN void panic(const char *format, ...);

/* Potentially recoverable error. */
void runtimeError(const char *format, ...);

/* Returns the current error string.
 * Returns NULL if runtimeError has never been called .*/
const char *getErrorString();

/* Clears the current error string so that 'getErrorString()' returns NULL */
void clearErrorString();

/* Adds a context provider that can add additional context to error messages.
 * The provider will be called for both 'panic' and 'runtimeError'. */
void setErrorContextProvider(void (*contextProvider)(StringBuffer*));

#endif/*mtots_util_error_h*/
