#include "mtots_ops.h"
#include "mtots_object.h"
#include "mtots_vm.h"

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

ubool valueLessThan(Value a, Value b) {
  if (a.type != b.type) {
    panic(
      "'<' requires values of the same type but got %s and %s",
      getKindName(a), getKindName(b));
  }
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) < AS_BOOL(b);
    case VAL_NIL: return UFALSE;
    case VAL_NUMBER: return AS_NUMBER(a) < AS_NUMBER(b);
    case VAL_CFUNCTION: break;
    case VAL_OPERATOR: break;
    case VAL_SENTINEL: break;
    case VAL_OBJ: {
      Obj *objA = AS_OBJ(a);
      Obj *objB = AS_OBJ(b);
      if (objA->type != objB->type) {
        panic(
          "'<' requires values of the same type but got %s and %s",
          getKindName(a), getKindName(b));
      }
      switch (objA->type) {
        case OBJ_STRING: {
          ObjString *strA = (ObjString*)objA;
          ObjString *strB = (ObjString*)objB;
          size_t lenA = strA->length;
          size_t lenB = strB->length;
          size_t len = lenA < lenB ? lenA : lenB;
          size_t i;
          const char *charsA = strA->chars;
          const char *charsB = strB->chars;
          if (strA == strB) {
            return UFALSE;
          }
          for (i = 0; i < len; i++) {
            if (charsA[i] != charsB[i]) {
              return charsA[i] < charsB[i];
            }
          }
          return lenA < lenB;
        }
        case OBJ_LIST: {
          ObjList *listA = (ObjList*)objA;
          ObjList *listB = (ObjList*)objB;
          size_t lenA = listA->length;
          size_t lenB = listB->length;
          size_t len = lenA < lenB ? lenA : lenB;
          size_t i;
          Value *bufA = listA->buffer;
          Value *bufB = listB->buffer;
          if (listA == listB) {
            return UFALSE;
          }
          for (i = 0; i < len; i++) {
            if (!valuesEqual(bufA[i], bufB[i])) {
              return valueLessThan(bufA[i], bufB[i]);
            }
          }
          return lenA < lenB;
        }
        default: break;
      }
      break;
    }
  }
  panic("%s does not support '<'", getKindName(a));
  return UFALSE;
}
