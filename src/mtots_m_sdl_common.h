#ifndef mtots_m_sdl_common_h
#define mtots_m_sdl_common_h

/* This header is meant to be included in the translation unit
 * for mtots_m_sdl.c */

#include "mtots_vm.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

static void nopBlacken(ObjNative *n) {}
static void nopFree(ObjNative *n) {}

/**********************************************************
 * strings
 *********************************************************/

typedef struct {
  ObjString **location;
  const char *value;
} RetainedString;

static ObjString *string_type;
static ObjString *string_key;
static ObjString *string_button;
static ObjString *string_timestamp;
static ObjString *string_repeat;
static ObjString *string_x;
static ObjString *string_y;
static ObjString *string_w;
static ObjString *string_h;
static ObjString *string_freq;
static ObjString *string_format;
static ObjString *string_channels;
static ObjString *string_samples;
static ObjString *string_callback;

static void mtots_m_SDL_initStrings(ObjInstance *module) {
  size_t i;
  ObjList *list;
  RetainedString rstrs[] = {
    {&string_type, "type"},
    {&string_button, "button"},
    {&string_timestamp, "timestamp"},
    {&string_key, "key"},
    {&string_repeat, "repeat"},
    {&string_x, "x"},
    {&string_y, "y"},
    {&string_w, "w"},
    {&string_h, "h"},
    {&string_freq, "freq"},
    {&string_format, "format"},
    {&string_channels, "channels"},
    {&string_samples, "samples"},
    {&string_callback, "callback"},
  };
  list = newList(sizeof(rstrs)/sizeof(RetainedString));
  tableSetN(&module->fields, "__retain__", OBJ_VAL(list));
  for (i = 0; i < sizeof(rstrs)/sizeof(RetainedString); i++) {
    *rstrs[i].location = copyCString(rstrs[i].value);
    list->buffer[i] = OBJ_VAL(*rstrs[i].location);
  }
}

/**********************************************************
 * TYPES AND CLASSES : structs for SDL types/classes
 *********************************************************/

typedef struct ObjWindow {
  ObjNative obj;
  SDL_Window *handle;
} ObjWindow;

typedef struct ObjEvent {
  ObjNative obj;
  SDL_Event data;
} ObjEvent;

typedef struct ObjKeyboardState {
  ObjNative obj;
  const Uint8 *state;
} ObjKeyboardState;

typedef struct ObjRect {
  ObjNative obj;
  SDL_Rect data;
} ObjRect;

typedef struct ObjPoint {
  ObjNative obj;
  SDL_Point handle;
} ObjPoint;

typedef struct ObjRenderer {
  ObjNative obj;
  SDL_Renderer *handle;
} ObjRenderer;

typedef struct ObjSurface {
  ObjNative obj;
  SDL_Surface *handle;
  Value pixelData; /* usually a ByteArray */
} ObjSurface;

typedef struct ObjTexture {
  ObjNative obj;
  SDL_Texture *handle;
} ObjTexture;

typedef struct ObjAudioSpec {
  ObjNative obj;
  SDL_AudioSpec data;
} ObjAudioSpec;

typedef struct ObjAudioDevice {
  ObjNative obj;
  SDL_AudioDeviceID handle;
} ObjAudioDevice;

/**********************************************************
 * TYPES AND CLASSES : descriptors (SDL types/classes)
 *
 * descriptors are (mostly) extern so that they can be forward
 * declared (due to historical reasons, static global
 * variables cannot be forward declared).
 *
 * POD types however, do not need to be forward declared
 *********************************************************/

extern NativeObjectDescriptor descriptorWindow;
extern NativeObjectDescriptor descriptorEvent;
extern NativeObjectDescriptor descriptorKeyboardState;
extern NativeObjectDescriptor descriptorRect;
extern NativeObjectDescriptor descriptorPoint;
extern NativeObjectDescriptor descriptorRenderer;
extern NativeObjectDescriptor descriptorSurface;
extern NativeObjectDescriptor descriptorTexture;
extern NativeObjectDescriptor descriptorAudioSpec;
extern NativeObjectDescriptor descriptorAudioDevice;

static NativeObjectDescriptor *descriptors[] = {
  &descriptorWindow,
  &descriptorEvent,
  &descriptorKeyboardState,
  &descriptorRect,
  &descriptorPoint,
  &descriptorRenderer,
  &descriptorSurface,
  &descriptorTexture,
  &descriptorAudioSpec,
  &descriptorAudioDevice,
};

#endif/*mtots_m_sdl_common_h*/
