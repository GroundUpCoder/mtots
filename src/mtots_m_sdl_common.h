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
static ObjString *string_repeat;
static ObjString *string_x;
static ObjString *string_y;
static ObjString *string_w;
static ObjString *string_h;

static void mtots_m_SDL_initStrings(ObjInstance *module) {
  size_t i;
  ObjList *list;
  RetainedString rstrs[] = {
    {&string_type, "type"},
    {&string_key, "key"},
    {&string_repeat, "repeat"},
    {&string_x, "x"},
    {&string_y, "y"},
    {&string_w, "w"},
    {&string_h, "h"},
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

typedef struct {
  ObjNative obj;
  const Uint8 *state;
} ObjKeyboardState;

typedef struct {
  ObjNative obj;
  SDL_Rect handle;
} ObjRect;

typedef struct {
  ObjNative obj;
  SDL_Renderer *handle;
} ObjRenderer;

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
extern NativeObjectDescriptor descriptorRenderer;

static NativeObjectDescriptor *descriptors[] = {
  &descriptorWindow,
  &descriptorEvent,
  &descriptorKeyboardState,
  &descriptorRect,
  &descriptorRenderer,
};

#endif/*mtots_m_sdl_common_h*/
