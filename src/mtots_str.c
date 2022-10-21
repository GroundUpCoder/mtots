#include "mtots_str.h"

#include "mtots_unicode.h"
#include "mtots_error.h"

#include <stdio.h>
#include <stdlib.h>

#define INVALID_HEX 24

static u32 evalHex(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
  if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
  return INVALID_HEX;
}

static char toHexChar(u32 digit) {
  if (digit < 10) {
    return '0' + digit;
  }
  return 'A' + (digit - 10);
}

void initStringEscapeOptions(StringEscapeOptions *opts) {
  opts->jsonSafe = UFALSE;
  opts->shorthandControlCodes = UTRUE;
  opts->tryUnicode = UTRUE;
}

/* TODO: Refactor to avoid passsing error messages back the same way
 * output is passed back. */
ubool escapeString(
    const char *str,
    size_t length,
    StringEscapeOptions *givenOpts,
    size_t *outLen,
    char *outBytes) {
  size_t len = 0, pos = 0;
  char *p = outBytes;
  StringEscapeOptions opts;

  if (givenOpts) opts = *givenOpts;
  else initStringEscapeOptions(&opts);

  while (pos < length) {
    int charlen;
    u32 codePoint;

    /* decode UTF-8 */
    charlen = decodeUTF8Char(str + pos, str + length, &codePoint);

    if (charlen == 0 || pos + charlen > length) {
      /* invalid UTF-8 code sequence */
      if (opts.jsonSafe) {
        /* if we need to be JSON safe, we emit an error */
        runtimeError("invalid unicode escape at %lu", (unsigned long)pos);
        return UFALSE;
      } else {
        /* otherwise, we can just emit a single byte */
        len += 4;
        if (p) {
          *p++ = '\\';
          *p++ = 'x';
          *p++ = toHexChar(((u8)str[pos] >> 4) % 16);
          *p++ = toHexChar(((u8)str[pos]) % 16);
        }
        pos++;
        continue;
      }
    }

    /* interpret */
    if (codePoint == '"' || codePoint == '\\') {
      /* Characters that must be escaped */
      len += 2;
      if (p) {
        *p++ = '\\';
        *p++ = codePoint;
      }
    } else if (opts.shorthandControlCodes && (
        (!opts.jsonSafe && codePoint == '\0') ||
        codePoint == '\b' ||
        codePoint == '\f' ||
        codePoint == '\n' ||
        codePoint == '\r' ||
        codePoint == '\t')) {
      /* Characters with shortcut escapes */
      len += 2;
      if (p) {
        *p++ = '\\';
        switch (codePoint) {
          case '\0': *p++ = '0'; break;
          case '\b': *p++ = 'b'; break;
          case '\f': *p++ = 'f'; break;
          case '\n': *p++ = 'n'; break;
          case '\r': *p++ = 'r'; break;
          case '\t': *p++ = 't'; break;
          default: abort();
        }
      }
    } else if (codePoint >= 0x20 && codePoint < 0x7F) {
      /* Printable ASCII */
      if (charlen != 1) abort();
      len++;
      if (p) *p++ = codePoint;
    } else if ((opts.jsonSafe||opts.tryUnicode) && codePoint <= 0xFFFF) {
      /* Escapable unicode in 16-bit range
       * Unconditionally escape them */
      len += 6;
      if (p) {
        *p++ = '\\';
        *p++ = 'u';
        *p++ = toHexChar((codePoint >> 12) % 16);
        *p++ = toHexChar((codePoint >> 8) % 16);
        *p++ = toHexChar((codePoint >> 4) % 16);
        *p++ = toHexChar(codePoint % 16);
      }
    } else if (!opts.jsonSafe && codePoint <= 0x7F) {
      /* All other ASCII
       * If we don't have to worry about whether we're JSON safe,
       * it makes sense to print these as just '\x' style byte sequences
       *
       * NOTE: it does not make sense to do this for codePoints that
       * are between 0x7F and 0xFF. This is because code points in
       * that range will in reality occupy 2 bytes in UTF-8, and so
       * will actually require
       *
       * In other words, this would be the range at which
       * \u00nn differs from \xnn. In such cases, we use one or the other
       * format depending on whether the caller requested 'tryUnicode'
       * or not.
       */
      len += 4;
      if (p) {
        *p++ = '\\';
        *p++ = 'x';
        *p++ = toHexChar(((u8)codePoint >> 4) % 16);
        *p++ = toHexChar(((u8)codePoint) % 16);
      }
    } else if ((opts.jsonSafe||opts.tryUnicode) && codePoint <= 0x10FFFF) {
      /* Other escapable unicode
       * However, these are out of the 16-bit range, so we
       * use a different syntax for them */
      len += 10;
      if (p) {
        *p++ = '\\';
        *p++ = 'U';
        *p++ = toHexChar((codePoint >> 28) % 16);
        *p++ = toHexChar((codePoint >> 24) % 16);
        *p++ = toHexChar((codePoint >> 20) % 16);
        *p++ = toHexChar((codePoint >> 16) % 16);
        *p++ = toHexChar((codePoint >> 12) % 16);
        *p++ = toHexChar((codePoint >> 8) % 16);
        *p++ = toHexChar((codePoint >> 4) % 16);
        *p++ = toHexChar(codePoint % 16);
      }
    } else if (opts.jsonSafe) {
      /* At this point, the codepoint is not valid, so we can't be json safe */
      runtimeError("invalid unicode codepoint %lu at %lu",
        (unsigned long) codePoint, (unsigned long)pos);
      return UFALSE;
    } else {
      /* Arbitrary bytes, and we don't have to worry about being jsonSafe */
      size_t i;
      for (i = 0; i < charlen; i++) {
        len += 4;
        if (p) {
          *p++ = '\\';
          *p++ = 'x';
          *p++ = toHexChar(((u8)str[pos + i] >> 4) % 16);
          *p++ = toHexChar(((u8)str[pos + i]) % 16);
        }
      }
    }

    /* incr */
    pos += charlen;
  }

  if (outLen) {
    *outLen = len;
  }
  return UTRUE;
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
      case '0': str++; len++; if (p) *p++ = '\0'; break;
      case 'x': {
        u32 byte;
        str++;
        if (evalHex(str[0]) == INVALID_HEX ||
            evalHex(str[1]) == INVALID_HEX) {
          char invalid = evalHex(str[0]) == INVALID_HEX ? str[0] : str[1];
          runtimeError("in string unescape, invalid hex digit '%c'", invalid);
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
          runtimeError("in string unescape, invalid hex digit '%c'", invalid);
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
      default:
        len = snprintf(NULL, 0,
          "in string unescape, invalid escape '%c' (%d)",
          *str, (int)*str);
        if (outLen) *outLen = len;
        if (outBytes) {
          snprintf(outBytes, len + 1,
            "in string unescape, invalid escape '%c' (%d)",
            *str, (int)*str);
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