#ifndef mtots_m_sdl_point_h
#define mtots_m_sdl_point_h

#include "mtots_m_sdl_common.h"

static ObjPoint *newPoint(int x, int y) {
  ObjPoint *pt = NEW_NATIVE(ObjPoint, &descriptorPoint);
  pt->handle.x = x;
  pt->handle.y = y;
  return pt;
}

static ubool implPoint(i16 argCount, Value *args, Value *out) {
  *out = OBJ_VAL_EXPLICIT((Obj*)newPoint(
    AS_NUMBER(args[0]),
    AS_NUMBER(args[1])));
  return UTRUE;
}

static TypePattern argsPoint[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcPoint = { implPoint, "Point",
  sizeof(argsPoint)/sizeof(TypePattern), 0, argsPoint };

static ubool pointGetField(ObjNative *n, String *key, Value *out) {
  ObjPoint *point = (ObjPoint*)n;
  if (key == string_x) {
    *out = NUMBER_VAL(point->handle.x);
    return UTRUE;
  } else if (key == string_y) {
    *out = NUMBER_VAL(point->handle.y);
    return UTRUE;
  }
  return UFALSE;
}

static ubool pointSetField(ObjNative *n, String *key, Value out) {
  ObjPoint *point = (ObjPoint*)n;
  if (!IS_NUMBER(out)) {
    panic("Point.%s requires a number value but got %s",
      key->chars, getKindName(out));
    return UFALSE;
  }
  if (key == string_x) {
    point->handle.x = AS_NUMBER(out);
    return UTRUE;
  } else if (key == string_y) {
    point->handle.y = AS_NUMBER(out);
    return UTRUE;
  }
  return UFALSE;
}

NativeObjectDescriptor descriptorPoint = {
  nopBlacken, nopFree, pointGetField, pointSetField, &funcPoint,
  sizeof(ObjPoint), "Point"};

#endif/*mtots_m_sdl_point_h*/
