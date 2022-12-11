#ifndef mtots_util_escape_h
#define mtots_util_escape_h

/* Some utilities for dealing with strings */

#include "mtots_common.h"
#include "mtots_util_strbuf.h"

typedef struct StringEscapeOptions {
  ubool jsonSafe;
  ubool shorthandControlCodes; /* Use shorthands for common control codes (e.g. '\n') */
  ubool tryUnicode;            /* Try to use unicode escapes */
} StringEscapeOptions;

/* Initializes StringEscapeOptions with defaults */
void initStringEscapeOptions(StringEscapeOptions *opts);

ubool escapeString2(
    StringBuffer *out,
    const char *str,
    size_t length,
    StringEscapeOptions *givenOpts);

/* Takes a string and escapes it as necessary
 * When NULL is passed to out* parameters, they are ignored
 * outLen: length of the final string not including the null terminator
 * outBytes: the buffer to write out the final string
 * */
ubool escapeString(
  const char *str,
  size_t len,
  StringEscapeOptions *opts,
  size_t *outLen,
  char *outBytes);

/* Takes in an escaped string (terminated with a '"') and computes its unescaped
 * version
 * When NULL is passed to out* parameters, they are ignored
 * outLen: length of the final string not including the null terminator
 * outBytes: the buffer to write out the final string */
ubool unescapeString(
  const char *str, const char *quote, size_t *outLen, char *outBytes);

ubool unescapeStringCharQuote(
  const char *str, char quote, size_t *outLen, char *outBytes);

#endif/*mtots_util_escape_h*/
