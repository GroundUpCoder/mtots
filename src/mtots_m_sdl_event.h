#ifndef mtots_m_sdl_event_h
#define mtots_m_sdl_event_h

#include "mtots_m_sdl_common.h"

static ubool implEvent(i16 argCount, Value *args, Value *out) {
  *out = OBJ_VAL(NEW_NATIVE(ObjEvent, &descriptorEvent));
  return UTRUE;
}

static CFunction funcEvent = { implEvent, "Event" , 0 };

static ubool eventGetField(ObjNative *n, ObjString *key, Value *out) {
  ObjEvent *event = (ObjEvent*)n;
  if (key == string_type) {
    *out = NUMBER_VAL(event->data.type);
    return UTRUE;
  } else if (key == string_key) {
    *out = NUMBER_VAL(event->data.key.keysym.sym);
    return UTRUE;
  } else if (key == string_repeat) {
    *out = BOOL_VAL(event->data.key.repeat != 0);
    return UTRUE;
  } else if (key == string_x) {
    switch (event->data.type) {
      case SDL_MOUSEWHEEL:
        *out = NUMBER_VAL(event->data.wheel.x);
        break;
      default:
        *out = NUMBER_VAL(event->data.button.x);
    }
    return UTRUE;
  } else if (key == string_y) {
    switch (event->data.type) {
      case SDL_MOUSEWHEEL:
        *out = NUMBER_VAL(event->data.wheel.y);
        break;
      default:
        *out = NUMBER_VAL(event->data.button.y);
    }
    return UTRUE;
  } else if (key == string_button) {
    *out = NUMBER_VAL(event->data.button.button);
    return UTRUE;
  } else if (key == string_timestamp) {
    *out = NUMBER_VAL(event->data.common.timestamp);
    return UTRUE;
  }
  return UFALSE;
}

NativeObjectDescriptor descriptorEvent = {
  nopBlacken, nopFree, eventGetField, NULL, &funcEvent,
  sizeof(ObjEvent), "Event"};

#endif/*mtots_m_sdl_event_h*/
