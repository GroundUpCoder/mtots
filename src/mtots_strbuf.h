#include "mtots_common.h"

typedef struct StringBuffer {
  char *chars;
  size_t length, capacity;
} StringBuffer;

void initStringBuffer(StringBuffer *sb);
void freeStringBuffer(StringBuffer *sb);
void sbputchar(StringBuffer *sb, char ch);
void sbprintf(StringBuffer *sb, const char *format, ...);
void StringBufferWriteNumber(StringBuffer *sb, double number);
