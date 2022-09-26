#ifndef mtotsobject_h
#define mtotsobject_h

#include <stddef.h>

#include "mtots.h"

typedef struct GCObject GCObject;

/*
 * Union of all mtots values
 */
typedef union Value {
  GCObject *gc;        /* collectable objects */
  void *p;             /* light userdata */
  mtots_CFunction f;   /* light C functions */
  int b;               /* bool */
  double n;            /* number */
} Value;

/*
 * Tagged Value
 */
typedef struct TValue {
  Value value;
  unsigned char kind;
} TValue;

struct GCObject {
  GCObject *next;
  unsigned char kind;
  unsigned char marked;
};

typedef struct String {
  GCObject header;
  size_t len;
  char contents[1];
} String;

typedef struct List {
  GCObject header;
  size_t len, cap;
  TValue *buffer;
} List;

typedef struct Table {
  GCObject header;
} Table;

typedef struct Function {
  GCObject header;
} Function;

#endif/*mtotsobject_h*/
