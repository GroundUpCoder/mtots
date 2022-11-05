#ifndef mtots_cfunc_h
#define mtots_cfunc_h

#include "mtots_common.h"
#include "mtots_ref.h"

typedef enum TypePatternType {
  TYPE_PATTERN_ANY = 0,
  TYPE_PATTERN_STRING_OR_NIL,
  TYPE_PATTERN_STRING,
  TYPE_PATTERN_BYTE_ARRAY,
  TYPE_PATTERN_BYTE_ARRAY_OR_VIEW,
  TYPE_PATTERN_BOOL,
  TYPE_PATTERN_NUMBER,
  TYPE_PATTERN_LIST_OR_NIL,
  TYPE_PATTERN_LIST,
  TYPE_PATTERN_DICT,
  TYPE_PATTERN_CLASS,
  TYPE_PATTERN_NATIVE_OR_NIL,
  TYPE_PATTERN_NATIVE
} TypePatternType;

typedef struct TypePattern {
  TypePatternType type;
  void *nativeTypeDescriptor;
} TypePattern;

typedef struct CFunc {
  ubool (*body)(i16 argCount, Ref args, Ref out);
  const char *name;
  i16 arity;
  i16 maxArity;
  TypePattern *argTypes;
  TypePattern receiverType;
} CFunc;

#endif/*mtots_cfunc_h*/
