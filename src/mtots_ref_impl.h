#ifndef mtots_ref_impl_h
#define mtots_ref_impl_h

#include "mtots_ref.h"
#include "mtots_ref_private.h"

#include "mtots_vm.h"

#define DEREF(x) ((vm.stack[(x).i]))

RefSet allocRefs(i16 n) {
  RefSet ret;
  if (n < 0) {
    panic("allocRefs requires non-negative argument but got %d", n);
  }
  if (vm.stackTop + n > vm.stack + STACK_MAX) {
    panic("stack overflow");
  }
  ret.start = vm.stackTop - vm.stack;
  ret.length = n;
  vm.stackTop += n;
  return ret;
}

Ref refAt(RefSet rs, i16 offset) {
  Ref ret;
  if (offset < 0 || offset >= rs.length) {
    panic("invalid offset for RefSet (length=%d, offset=%d)",
      rs.length, offset);
  }
  ret.i = rs.start + offset;
  return ret;
}

Ref allocRef() {
  return refAt(allocRefs(1), 0);
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

void setString(Ref out, const char *value, size_t byteLength) {
  DEREF(out) = OBJ_VAL(copyString(value, byteLength));
}

void setCString(Ref out, const char *value) {
  DEREF(out) = OBJ_VAL(copyCString(value));
}

void setEmptyList(Ref out) {
  DEREF(out) = OBJ_VAL(newList(0));
}

void setList(Ref out, RefSet items) {
  ObjList *list = newList(items.length);
  DEREF(out) = OBJ_VAL(list);
  memcpy(list->buffer, vm.stack + items.start, sizeof(Value) * items.length);
}

void setInstanceField(Ref recv, const char *fieldName, Ref value) {
  ObjInstance *instance;
  if (!IS_INSTANCE(DEREF(recv))) {
    panic("Expected instance but got %s", getKindName(DEREF(recv)));
  }
  instance = AS_INSTANCE(DEREF(recv));
  mapSetN(&instance->fields, fieldName, DEREF(value));
}

Ref allocNil() {
  Ref r = allocRef();
  setNil(r);
  return r;
}

Ref allocBool(ubool boolean) {
  Ref r = allocRef();
  setBool(r, boolean);
  return r;
}

Ref allocNumber(double number) {
  Ref r = allocRef();
  setNumber(r, number);
  return r;
}

Ref allocString(const char *string, size_t length) {
  Ref r = allocRef();
  setString(r, string, length);
  return r;
}

Ref allocCString(const char *string) {
  Ref r = allocRef();
  setCString(r, string);
  return r;
}

Ref allocEmptyList() {
  Ref r = allocRef();
  setEmptyList(r);
  return r;
}

Ref allocList(RefSet items) {
  Ref r = allocRef();
  setList(r, items);
  return r;
}

Ref allocDict() {
  Ref r = allocRef();
  setDict(r);
  return r;
}

Ref allocValue(Ref src) {
  Ref r = allocRef();
  setValue(r, src);
  return r;
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

size_t stringSize(Ref r) {
  if (!IS_STRING(DEREF(r))) {
    panic("Expected string but got %s", getKindName(DEREF(r)));
  }
  return AS_STRING(DEREF(r))->length;
}

Value refGet(Ref ref) {
  return DEREF(ref);
}

void refSet(Ref ref, Value value) {
  DEREF(ref) = value;
}

#endif/*mtots_ref_impl_h*/
