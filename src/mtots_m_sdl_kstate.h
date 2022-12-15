#ifndef mtots_m_sdl_kstate_h
#define mtots_m_sdl_kstate_h

#include "mtots_m_sdl_common.h"

static ubool implKeyboardStateGetItem(i16 argCount, Value *args, Value *out) {
  ObjKeyboardState *kstate;
  size_t index;
  kstate = (ObjKeyboardState*)AS_OBJ(args[-1]);
  if (!IS_NUMBER(args[0])) {
    runtimeError("Expected number but got %s", getKindName(args[0]));
    return UFALSE;
  }
  index = (size_t) AS_NUMBER(args[0]);
  if (index > (size_t)(kstate->numKeys)) {
    runtimeError(
      "KeyboardState index out of bounds (index=%lu, numKeys=%d)",
      (unsigned long)index,
      kstate->numKeys);
    return UFALSE;
  }
  *out = BOOL_VAL(kstate->state[index] != 0);
  return UTRUE;
}

static TypePattern argsKeyboardStateGetItem[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcKeyboardStateGetItem = {
  implKeyboardStateGetItem, "__getitem__",
  sizeof(argsKeyboardStateGetItem) / sizeof(TypePattern), 0,
  argsKeyboardStateGetItem };

static CFunction *keyboardStateMethods[] = {
  &funcKeyboardStateGetItem,
  NULL,
};

NativeObjectDescriptor descriptorKeyboardState = {
  nopBlacken, nopFree, NULL, NULL, NULL,
  sizeof(ObjKeyboardState), "KeyboardState",
  keyboardStateMethods };

#endif/*mtots_m_sdl_kstate_h*/
