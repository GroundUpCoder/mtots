#ifndef mtots_ref_impl_h
#define mtots_ref_impl_h

#include "mtots_ref.h"

#include "mtots_vm.h"

#define DEREF(x) ((vm.stack[(x).i]))

Ref allocRefs(i16 n) {
  Ref ret;
  if (n <= 0) {
    panic("allocRefs requires positive argument but got %d", n);
  }
  if (vm.stackTop + n > vm.stack + STACK_MAX) {
    panic("stack overflow");
  }
  ret.i = vm.stackTop - vm.stack;
  vm.stackTop += n;
  return ret;
}

Ref refAt(Ref base, i16 offset) {
  base.i += offset;
  return base;
}

StackState getStackState() {
  StackState state;
  state.size = vm.stackTop - vm.stack;
  return state;
}

void restoreStackState(StackState state) {
  if (vm.stack + state.size > vm.stackTop) {
    panic("restoreStackState: inconsistent stack state");
  }
  vm.stackTop = vm.stack + state.size;
}

ubool isNil(Ref r) {
  return IS_NIL(DEREF(r));
}

ubool isBool(Ref r) {
  return IS_BOOL(DEREF(r));
}

ubool isNumber(Ref r) {
  return IS_NUMBER(DEREF(r));
}

ubool isCFunc(Ref r) {
  return IS_CFUNC(DEREF(r));
}

ubool isObj(Ref r) {
  return IS_OBJ(DEREF(r));
}

ubool isClass(Ref r) {
  return IS_OBJ(DEREF(r)) && IS_CLASS(DEREF(r));
}

ubool isClosure(Ref r) {
  return IS_OBJ(DEREF(r)) && IS_CLOSURE(DEREF(r));
}

ubool isString(Ref r) {
  return IS_OBJ(DEREF(r)) && IS_STRING(DEREF(r));
}

ubool isList(Ref r) {
  return IS_OBJ(DEREF(r)) && IS_LIST(DEREF(r));
}

ubool isTuple(Ref r) {
  return IS_OBJ(DEREF(r)) && IS_TUPLE(DEREF(r));
}

ubool isFile(Ref r) {
  return IS_OBJ(DEREF(r)) && IS_FILE(DEREF(r));
}

void setNil(Ref out) {
  DEREF(out) = NIL_VAL();
}

void setBool(Ref out, ubool value) {
  DEREF(out) = BOOL_VAL(value);
}

void setNumber(Ref out, double value) {
  DEREF(out) = NUMBER_VAL(value);
}

void setCFunc(Ref out, CFunc *value) {
  DEREF(out) = CFUNC_VAL(value);
}

void setString(Ref out, const char *value) {
  DEREF(out) = OBJ_VAL(copyCString(value));
}

void setStringWithLength(Ref out, const char *value, size_t byteLength) {
  DEREF(out) = OBJ_VAL(copyString(value, byteLength));
}

void setInstanceField(Ref recv, const char *fieldName, Ref value) {
  ObjInstance *instance;
  if (!IS_INSTANCE(DEREF(recv))) {
    panic("Expected instance but got %s", getKindName(DEREF(recv)));
  }
  instance = AS_INSTANCE(DEREF(recv));
  dictSetN(&instance->fields, fieldName, DEREF(value));
}

void getClass(Ref out, Ref value) {
  DEREF(out) = OBJ_VAL(getClassOfValue(vm.stack[value.i]));
}

ubool getBool(Ref r) {
  if (!IS_BOOL(DEREF(r))) {
    panic("Expected bool but got %s", getKindName(DEREF(r)));
  }
  return AS_BOOL(DEREF(r));
}

double getNumber(Ref r) {
  if (!IS_NUMBER(DEREF(r))) {
    panic("Expected number but got %s", getKindName(DEREF(r)));
  }
  return AS_NUMBER(DEREF(r));
}

const char *getString(Ref r) {
  if (!IS_STRING(DEREF(r))) {
    panic("Expected string but got %s", getKindName(DEREF(r)));
  }
  return AS_STRING(DEREF(r))->chars;
}

size_t getStringByteLength(Ref r) {
  if (!IS_STRING(DEREF(r))) {
    panic("Expected string but got %s", getKindName(DEREF(r)));
  }
  return AS_STRING(DEREF(r))->length;
}

#endif/*mtots_ref_impl_h*/
