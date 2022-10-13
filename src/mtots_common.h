#ifndef mtots_common_h
#define mtots_common_h

#include "mtots_config.h"
#include <stddef.h>

#define MTOTS_EXIT_CODE_OK 0
#define MTOTS_EXIT_CODE_ASSERTION_FAILED 1  /* NOTE: prefer abort() */
#define MTOTS_EXIT_CODE_UNKNOWN_ERROR 2     /* NOTE: prefer abort() */
#define MTOTS_EXIT_CODE_COULD_NOT_OPEN_SCRIPT_FILE 3
#define MTOTS_EXIT_CODE_COULD_NOT_READ_SCRIPT_FILE 4
#define MTOTS_EXIT_CODE_OOM 10
#define MTOTS_EXIT_CODE_OOM_SCRIPT_FILE 11
#define MTOTS_EXIT_CODE_STACK_OVERFLOW 12
#define MTOTS_EXIT_CODE_STACK_UNDERFLOW 13
#define MTOTS_EXIT_CODE_NOT_ENOUGH_MEMORY_FOR_SCRIPT_FILE 14
#define MTOTS_EXIT_CODE_OOM_GC 15
#define MTOTS_EXIT_CODE_COMPILE_ERROR 21
#define MTOTS_EXIT_CODE_RUNTIME_ERROR 22
#define MTOTS_EXIT_CODE_PANIC 23
#define U8_MAX  255
#define U16_MAX 65535

#define UTRUE 1
#define UFALSE 0

#define U8_COUNT (U8_MAX + 1)
#define MAX_ARG_COUNT 127

/* PLATFORM DEPENDENT NOTE: Strictly speaking, C89 does not
 * guarantee that char always holds 8 bits (e.g. on some
 * ancient platforms a byte may be 7-bits).
 * Further short is not guaranteed to be exactly 2 bytes.
 *
 * However, on almost every modern platform, a byte is
 * 8 bits, and short is 2 bytes.
 *
 * Assuming int is 32-bits may seem a bit more of a stretch,
 * but actually ILP64 are relatively less common. Most 64-bit
 * platforms seem to prefer LP64 where int stays 32-bits and
 * only long ints and pointers are 64 bits.
 * See https://unix.org/version2/whatsnew/lp64_wp.html
 * "64-Bit Programming Models: Why LP64?"
 *
 * Even Win64 is a LLP64 model which means int is still 32 bits:
 * https://devblogs.microsoft.com/oldnewthing/20050131-00/?p=36563
 */
typedef unsigned char  u8;
typedef   signed char  i8;
typedef unsigned short u16;
typedef   signed short i16;
typedef unsigned int   u32;
typedef   signed int   i32;

typedef u8 ubool;

typedef float  f32;
typedef double f64;

#endif/*mtots_common_h*/
