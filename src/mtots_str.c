#include "mtots_str.h"

#include "mtots_unicode.h"

#include <stdio.h>

#define INVALID_HEX 24

static u32 evalHex(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
  if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
  return INVALID_HEX;
}

ubool escapeString(const char *str, size_t *outLen, char *outBytes) {
  return UFALSE; /* TODO */
}


ubool unescapeString(const char *str, char quote, size_t *outLen, char *outBytes) {
  size_t len = 0;
  char *p = outBytes;

  while (*str != quote && *str != '\0') {
    if (*str == '\\') {
      str++;
      switch (*str) {
      case '"': str++; len++; if (p) *p++ = '\"'; break;
      case '\'': str++; len++; if (p) *p++ = '\''; break;
      case '\\': str++; len++; if (p) *p++ = '\\'; break;
      case '/': str++; len++; if (p) *p++ = '/'; break;
      case 'b': str++; len++; if (p) *p++ = '\b'; break;
      case 'f': str++; len++; if (p) *p++ = '\f'; break;
      case 'n': str++; len++; if (p) *p++ = '\n'; break;
      case 'r': str++; len++; if (p) *p++ = '\r'; break;
      case 't': str++; len++; if (p) *p++ = '\t'; break;
      case 'x': {
        u32 byte;
        str++;
        if (evalHex(str[0]) == INVALID_HEX ||
            evalHex(str[1]) == INVALID_HEX) {
          char invalid = evalHex(str[0]) == INVALID_HEX ? str[0] : str[1];
          *outLen = snprintf(NULL, 0,
            "in string unescape, invalid hex digit '%c'", invalid);
          if (outBytes) {
            snprintf(
              outBytes, *outLen + 1,
              "in string unescape, invalid hex digit '%c'", invalid);
          }
          return UFALSE;
        }
        byte = evalHex(str[0]) << 4 | evalHex(str[1]);
        if (p) *p++ = (char)(u8)byte;
        len++;
        str += 2; /* 2 hex digits we just processed */
        break;
      }
      case 'u': {
        int charBytes;
        u32 codePoint;
        str++;
        if (evalHex(str[0]) == INVALID_HEX ||
            evalHex(str[1]) == INVALID_HEX ||
            evalHex(str[2]) == INVALID_HEX ||
            evalHex(str[3]) == INVALID_HEX) {
          char invalid =
            evalHex(str[0]) == INVALID_HEX ? str[0] :
            evalHex(str[1]) == INVALID_HEX ? str[1] :
            evalHex(str[2]) == INVALID_HEX ? str[2] : str[3];
          if (outLen) {
            *outLen = snprintf(NULL, 0,
              "in string unescape, invalid hex digit '%c'", invalid);
          }
          if (outBytes) {
            snprintf(
              outBytes, *outLen + 1,
              "in string unescape, invalid hex digit '%c'", invalid);
          }
          return UFALSE;
        }
        codePoint =
          evalHex(str[0]) << 12 |
          evalHex(str[1]) << 8 |
          evalHex(str[2]) << 4 |
          evalHex(str[3]);
        charBytes = encodeUTF8Char(codePoint, p);
        if (p) p += charBytes;
        len += charBytes;
        str += 4; /* 4 hex digits we just processed */
        break;
      }
      }
    } else {
      len++;
      if (p) *p++ = *str;
      str++;
    }
  }

  if (outLen) {
    *outLen = len;
  }

  return UTRUE;
}
