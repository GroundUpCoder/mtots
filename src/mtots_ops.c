#include "mtots_ops.h"
#include "mtots_object.h"

#include <stdlib.h>
#include <string.h>

ubool valuesIs(Value a, Value b) {
  if (a.type != b.type) {
    return UFALSE;
  }
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL: return UTRUE;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_CFUNCTION: return AS_CFUNCTION(a) == AS_CFUNCTION(b);
    case VAL_OPERATOR: return AS_OPERATOR(a) == AS_OPERATOR(b);
    case VAL_SENTINEL: return AS_SENTINEL(a) == AS_SENTINEL(b);
    case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);
  }
  abort();
  return UFALSE; /* Unreachable */
}

ubool valuesEqual(Value a, Value b) {
  if (a.type != b.type) {
    return UFALSE;
  }
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL: return UTRUE;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_CFUNCTION: return AS_CFUNCTION(a) == AS_CFUNCTION(b);
    case VAL_OPERATOR: return AS_OPERATOR(a) == AS_OPERATOR(b);
    case VAL_SENTINEL: return AS_SENTINEL(a) == AS_SENTINEL(b);
    case VAL_OBJ: {
      Obj *objA = AS_OBJ(a);
      Obj *objB = AS_OBJ(b);
      if (objA->type != objB->type) {
        return UFALSE;
      }
      switch (objA->type) {
        case OBJ_BYTE_ARRAY: {
          ObjByteArray *aa = (ObjByteArray*)objA, *ab = (ObjByteArray*)objB;
          if (aa->size != ab->size) {
            return UFALSE;
          }
          return memcmp(aa->buffer, ab->buffer, aa->size) == 0;
        }
        case OBJ_LIST: {
          ObjList *listA = (ObjList*)objA, *listB = (ObjList*)objB;
          size_t i;
          if (listA->length != listB->length) {
            return UFALSE;
          }
          for (i = 0; i < listA->length; i++) {
            if (!valuesEqual(listA->buffer[i], listB->buffer[i])) {
              return UFALSE;
            }
          }
          return UTRUE;
        }
        case OBJ_DICT: {
          /* TODO */
          return objA == objB;
        }
        default: return objA == objB;
      }
    }
  }
  abort();
  return UFALSE; /* Unreachable */
}
