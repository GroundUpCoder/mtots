#include "mtots_object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* should-be-inline */ u32 AS_U32(Value value) {
  double x = AS_NUMBER(value);
  return x < 0 ? ((u32) (i32) x) : (u32) x;
}

/* should-be-inline */ i32 AS_I32(Value value) {
  return (i32) AS_NUMBER(value);
}

Value BOOL_VAL(ubool value) {
  Value v = {VAL_BOOL};
  v.as.boolean = value;
  return v;
}
Value NIL_VAL() {
  Value v = {VAL_NIL};
  return v;
}
Value NUMBER_VAL(double value) {
  Value v = {VAL_NUMBER};
  v.as.number = value;
  return v;
}
Value STRING_VAL(String *string) {
  Value v = { VAL_STRING };
  v.as.string = string;
  return v;
}
Value CFUNCTION_VAL(CFunction *func) {
  Value v = {VAL_CFUNCTION};
  v.as.cfunction = func;
  return v;
}
Value OPERATOR_VAL(Operator op) {
  Value v = {VAL_OPERATOR};
  v.as.op = op;
  return v;
}
Value SENTINEL_VAL(Sentinel sentinel) {
  Value v = {VAL_SENTINEL};
  v.as.sentinel = sentinel;
  return v;
}
Value OBJ_VAL_EXPLICIT(Obj *object) {
  Value v = {VAL_OBJ};
  v.as.obj = object;
  return v;
}

void initValueArray(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void writeValueArray(ValueArray *array, Value value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(
      Value, array->values, oldCapacity, array->capacity);
  }

  array->values[array->count++] = value;
}

void freeValueArray(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array);
}

void printValue(Value value) {
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      return;
    case VAL_NIL:
      printf("nil");
      return;
    case VAL_NUMBER:
      printf("%g", AS_NUMBER(value));
      return;
    case VAL_STRING:
      printf("%s", AS_CSTRING(value));
      return;
    case VAL_CFUNCTION: {
      CFunction *fn = AS_CFUNCTION(value);
      printf("<function %s at %p>", fn->name, (void*)fn);
      return;
    }
    case VAL_OPERATOR: {
      Operator op = AS_OPERATOR(value);
      switch (op) {
        case OperatorLen: printf("<operator len>"); return;
      }
      printf("<operator unknown(%d)>", op);
      return;
    }
    case VAL_SENTINEL: printf("<sentinel %d>", AS_SENTINEL(value)); return;
    case VAL_OBJ:
      printObject(value);
      return;
  }
  printf("UNRECOGNIZED(%d)", value.type);
}

const char *getValueTypeName(ValueType type) {
  switch (type) {
    case VAL_BOOL: return "VAL_BOOL";
    case VAL_NIL: return "VAL_NIL";
    case VAL_NUMBER: return "VAL_NUMBER";
    case VAL_STRING: return "VAL_STRING";
    case VAL_CFUNCTION: return "VAL_CFUNCTION";
    case VAL_OPERATOR: return "VAL_OPERATOR";
    case VAL_SENTINEL: return "VAL_SENTINEL";
    case VAL_OBJ: return "VAL_OBJ";
  }
  return "<unrecognized>";
}

/* Returns a human readable string describing the 'kind' of the given value.
 * For non-object values, a string describing its value type is returned.
 * For object values, a string describing its object type is returned.
 */
const char *getKindName(Value value) {
  switch (value.type) {
    case VAL_BOOL: return "bool";
    case VAL_NIL: return "nil";
    case VAL_NUMBER: return "number";
    case VAL_STRING: return "string";
    case VAL_CFUNCTION: return "cfunction";
    case VAL_OPERATOR: return "operator";
    case VAL_SENTINEL: return "sentinel";
    case VAL_OBJ: switch (value.as.obj->type) {
      case OBJ_CLASS: return "class";
      case OBJ_CLOSURE: return "closure";
      case OBJ_THUNK: return "function";
      case OBJ_NATIVE_CLOSURE: return "native-closure";
      case OBJ_INSTANCE: return "instance";
      case OBJ_BYTE_ARRAY: return "byteArray";
      case OBJ_LIST: return "list";
      case OBJ_TUPLE: return "tuple";
      case OBJ_DICT: return "dict";
      case OBJ_FILE: return "file";
      case OBJ_NATIVE: return AS_NATIVE(value)->descriptor->name;
      case OBJ_UPVALUE: return "upvalue";
      default: return "<unrecognized-object>";
    }
  }
  return "<unrecognized-value>";
}

ubool typePatternMatch(TypePattern pattern, Value value) {
  switch (pattern.type) {
    case TYPE_PATTERN_ANY: return UTRUE;
    case TYPE_PATTERN_STRING_OR_NIL:
      if (IS_NIL(value)) {
        return UTRUE;
      }
      /* fallthrough */
    case TYPE_PATTERN_STRING: return IS_STRING(value);
    case TYPE_PATTERN_BYTE_ARRAY: return IS_BYTE_ARRAY(value);
    case TYPE_PATTERN_BYTE_ARRAY_OR_VIEW:
      return IS_BYTE_ARRAY(value) || IS_BYTE_ARRAY_VIEW(value);
    case TYPE_PATTERN_BOOL: return IS_BOOL(value);
    case TYPE_PATTERN_NUMBER: return IS_NUMBER(value);
    case TYPE_PATTERN_LIST_OR_NIL:
      if (IS_NIL(value)) {
        return UTRUE;
      }
      /* fallthrough */
    case TYPE_PATTERN_LIST: return IS_LIST(value);
    case TYPE_PATTERN_DICT: return IS_DICT(value);
    case TYPE_PATTERN_CLASS: return IS_CLASS(value);
    case TYPE_PATTERN_NATIVE_OR_NIL:
      if (IS_NIL(value)) {
        return UTRUE;
      }
      /* fallthrough */
    case TYPE_PATTERN_NATIVE:
      return IS_NATIVE(value) && (
        pattern.nativeTypeDescriptor == NULL ||
        AS_NATIVE(value)->descriptor == pattern.nativeTypeDescriptor);
  }
  panic("Unrecognized TypePattern type %d", pattern.type);
  return UFALSE;
}

const char *getTypePatternName(TypePattern pattern) {
  switch (pattern.type) {
    case TYPE_PATTERN_ANY: return "any";
    case TYPE_PATTERN_STRING_OR_NIL: return "(string|nil)";
    case TYPE_PATTERN_STRING: return "string";
    case TYPE_PATTERN_BYTE_ARRAY: return "ByteArray";
    case TYPE_PATTERN_BYTE_ARRAY_OR_VIEW: return "(ByteArray|ByteArrayView)";
    case TYPE_PATTERN_BOOL: return "bool";
    case TYPE_PATTERN_NUMBER: return "number";
    case TYPE_PATTERN_LIST_OR_NIL: return "(list|nil)";
    case TYPE_PATTERN_LIST: return "list";
    case TYPE_PATTERN_DICT: return "dict";
    case TYPE_PATTERN_CLASS: return "class";
    case TYPE_PATTERN_NATIVE_OR_NIL:
      return pattern.nativeTypeDescriptor ?
        ((NativeObjectDescriptor*) pattern.nativeTypeDescriptor)->name :
        "(native|nil)";
    case TYPE_PATTERN_NATIVE:
      return pattern.nativeTypeDescriptor ?
        ((NativeObjectDescriptor*) pattern.nativeTypeDescriptor)->name :
        "native";
  }
  panic("Unrecognized TypePattern type %d", pattern.type);
  return UFALSE;
}
