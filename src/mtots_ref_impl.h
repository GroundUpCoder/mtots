#ifndef mtots_ref_impl_h
#define mtots_ref_impl_h

#include "mtots_ref.h"

#include "mtots_vm.h"

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
    panic("restoreStackState - inconsistent stack state");
  }
  vm.stackTop = vm.stack + state.size;
}

void setNil(Ref out) {
  vm.stack[out.i] = NIL_VAL();
}

void setBool(Ref out, ubool value) {
  vm.stack[out.i] = BOOL_VAL(value);
}

void setNumber(Ref out, double value) {
  vm.stack[out.i] = NUMBER_VAL(value);
}

void setString(Ref out, const char *value) {
  vm.stack[out.i] = OBJ_VAL(copyCString(value));
}

void refGetClass(Ref out, Ref value) {
  vm.stack[out.i] = OBJ_VAL(getClass(vm.stack[value.i]));
}

#endif/*mtots_ref_impl_h*/
