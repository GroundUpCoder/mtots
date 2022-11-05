#ifndef mtots_m_sdl_common_h
#define mtots_m_sdl_common_h

/* This header is meant to be included in the translation unit
 * for mtots_m_sdl.c */

#include "mtots_vm.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#define AUDIO_SAMPLE_RATE          44100
#define AUDIO_SAMPLES_BUFFER_SIZE    512
#define AUDIO_TRACK_COUNT              8

static void nopBlacken(ObjNative *n) {}
static void nopFree(ObjNative *n) {}

typedef enum WaveForm {
  WAVE_FORM_SINE,
  WAVE_FORM_SAWTOOTH,
  WAVE_FORM_SQUARE,
  WAVE_FORM_TRIANGLE,
  WAVE_FORM_COUNT
} WaveForm;

/**********************************************************
 * strings
 *********************************************************/

typedef struct RetainedString {
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
  };
  list = newList(sizeof(rstrs)/sizeof(RetainedString));
  dictSetN(&module->fields, "__retain__", OBJ_VAL(list));
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
  &descriptorAudioDevice,
};

#endif/*mtots_m_sdl_common_h*/
