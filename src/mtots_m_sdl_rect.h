#ifndef mtots_m_sdl_rect_h
#define mtots_m_sdl_rect_h

#include "mtots_m_sdl_common.h"

static ObjRect *newRect(int x, int y, int w, int h) {
  ObjRect *rect = NEW_NATIVE(ObjRect, &descriptorRect);
  rect->handle.x = x;
  rect->handle.y = y;
  rect->handle.w = w;
  rect->handle.h = h;
  return rect;
}

static ubool implRect(i16 argCount, Value *args, Value *out) {
  size_t i;
  for (i = 0; i < 4; i++) {
    if (!IS_NUMBER(args[i])) {
      runtimeError("Rect() requires number arguments but got %s",
        getKindName(args[i]));
      return UFALSE;
    }
  }
  *out = OBJ_VAL(newRect(
    AS_NUMBER(args[0]),
    AS_NUMBER(args[1]),
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3])));
  return UTRUE;
}

static CFunction funcRect = { implRect, "Rect", 4 };

static ubool rectGetField(ObjNative *n, ObjString *key, Value *out) {
  ObjRect *rect = (ObjRect*)n;
  if (key == string_x) {
    *out = NUMBER_VAL(rect->handle.x);
    return UTRUE;
  } else if (key == string_y) {
    *out = NUMBER_VAL(rect->handle.y);
    return UTRUE;
  } else if (key == string_w) {
    *out = NUMBER_VAL(rect->handle.w);
    return UTRUE;
  } else if (key == string_h) {
    *out = NUMBER_VAL(rect->handle.h);
    return UTRUE;
  }
  return UFALSE;
}

NativeObjectDescriptor descriptorRect = {
  nopBlacken, nopFree, rectGetField, &funcRect,
  sizeof(ObjRect), "Rect" };

#endif/*mtots_m_sdl_rect_h*/
