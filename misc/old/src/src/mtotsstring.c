#include "mtotsstring.h"

#include <stdio.h>

static size_t count_percent(const char *fmt) {
  size_t ret = 0;
  for (; *fmt; fmt++) {
    if (*fmt == '%') {
      ret++;
    }
  }
  return ret;
}

const char *mkfmt(mtots_State *L, const char *fmt, ...) {
  const char *ret;
  va_list args;
  va_start(args, fmt);
  ret = mkfmtV(L, fmt, args);
  va_end(args);
  return ret;
}

const char *mkfmtV(mtots_State *L, const char *fmt, va_list args) {
  char buffer[64];
  size_t cap = strlen(fmt) + count_percent(fmt) * 8 + 2;
  size_t len = 0;
  char *str = (char*) malloc(cap);
  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt++) {
        case '\0':
          mtots_panic(L, "'%' encountered at EOS");
        case '%':
          if (len + 2 >= cap) {
            cap *= 2;
            str = (char*) realloc(str, cap);
          }
          str[len++] = '%';
          break;
        case 's': {
          const char *s = va_arg(args, const char*);
          size_t slen = strlen(s);
          while (len + slen + 1 >= cap) {
            cap *= 2;
            str = (char*) realloc(str, cap);
          }
          strcpy(str + len, s);
          len += slen;
          break;
        }
        case 'f': {
          double value = va_arg(args, double);
          size_t slen = snprintf(buffer, 64, "%f", value);
          while (buffer[slen - 1] == '0') {
            buffer[--slen] = '\0';
          }
          if (buffer[slen - 1] == '.') {
            buffer[--slen] = '\0';
          }
          strcpy(str + len, buffer);
          len += slen;
          break;
        }
        case 'd':
        case 'i': {
          int  value = va_arg(args, int);
          size_t slen = snprintf(buffer, 64, "%d", value);
          strcpy(str + len, buffer);
          len += slen;
          break;
        }
        default: {
            char msg[] = "Unrecognized modifier %%";
            msg[strlen(msg) - 1] = *(fmt - 1);
            mtots_panic(L, msg);
          }
      }
    } else {
      if (len + 2 >= cap) {
        cap *= 2;
        str = (char*) realloc(str, cap);
      }
      str[len++] = *fmt++;
    }
  }
  str[len] = '\0';
  return str;
}
