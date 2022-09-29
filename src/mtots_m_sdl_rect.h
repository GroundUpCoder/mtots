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
  *out = OBJ_VAL(newRect(
    AS_NUMBER(args[0]),
    AS_NUMBER(args[1]),
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3])));
  return UTRUE;
}

static TypePattern argsRect[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcRect = { implRect, "Rect",
  sizeof(argsRect)/sizeof(TypePattern), 0, argsRect };

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

static ubool rectSetField(ObjNative *n, ObjString *key, Value out) {
  ObjRect *rect = (ObjRect*)n;
  if (!IS_NUMBER(out)) {
    panic("Rect.%s requires a number value but got %s",
      key->chars, getKindName(out));
    return UFALSE;
  }
  if (key == string_x) {
    rect->handle.x = AS_NUMBER(out);
    return UTRUE;
  } else if (key == string_y) {
    rect->handle.y = AS_NUMBER(out);
    return UTRUE;
  } else if (key == string_w) {
    rect->handle.w = AS_NUMBER(out);
    return UTRUE;
  } else if (key == string_h) {
    rect->handle.h = AS_NUMBER(out);
    return UTRUE;
  }
  return UFALSE;
}

NativeObjectDescriptor descriptorRect = {
  nopBlacken, nopFree, rectGetField, rectSetField, &funcRect,
  sizeof(ObjRect), "Rect" };

#endif/*mtots_m_sdl_rect_h*/
