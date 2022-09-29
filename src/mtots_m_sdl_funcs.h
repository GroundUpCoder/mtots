#ifndef mtots_m_sdl_funcs_h
#define mtots_m_sdl_funcs_h

#include "mtots_m_sdl_common.h"

/**********************************************************
 * functions: Initialization and Startup
 *********************************************************/

static ubool implInit(i16 argCount, Value *args, Value *out) {
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    runtimeError("Failed to init SDL video: %s", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static CFunction funcInit = { implInit, "init", 0 };

static ubool implQuit(i16 argCount, Value *args, Value *out) {
  SDL_Quit();
  return UTRUE;
}

static CFunction funcQuit = { implQuit, "quit", 0 };

static ubool implCreateWindow(i16 argCount, Value *args, Value *out) {
  size_t i;
  ObjWindow *window;
  if (!IS_STRING(args[0])) {
    runtimeError(
      "createWindow() title must be a string but got %s",
      getKindName(args[0]));
    return UFALSE;
  }
  for (i = 1; i < argCount; i++) {
    if (!IS_NUMBER(args[i])) {
      runtimeError(
        "createWindow() coordinates must be a number, but got %s",
        getKindName(args[i]));
      return UFALSE;
    }
  }
  window = NEW_NATIVE(ObjWindow, &descriptorWindow);
  window->handle = SDL_CreateWindow(
    AS_STRING(args[0])->chars,
    (int) (u32) AS_NUMBER(args[1]),
    (int) (u32) AS_NUMBER(args[2]),
    (int) (u32) AS_NUMBER(args[3]),
    (int) (u32) AS_NUMBER(args[4]),
    (u32) AS_NUMBER(args[5]));
  *out = OBJ_VAL(window);
  return UTRUE;
}

static CFunction funcCreateWindow = { implCreateWindow, "createWindow", 6 };

/**********************************************************
 * functions: Input
 *********************************************************/

static ubool implPollEvent(i16 argCount, Value *args, Value *out) {
  ObjEvent *event;
  if (!IS_NATIVE(args[0]) ||
      AS_NATIVE(args[0])->descriptor != &descriptorEvent) {
    runtimeError(
      "sdl.pollEvent() requires an sdl.Event argument but got %s",
      getKindName(args[0]));
    return UFALSE;
  }
  event = (ObjEvent*)AS_OBJ(args[0]);
  *out = BOOL_VAL(SDL_PollEvent(&event->data));
  return UTRUE;
}

static CFunction funcPollEvent = { implPollEvent, "pollEvent", 1 };

static ubool implGetKeyboardState(i16 argCount, Value *args, Value *out) {
  ObjKeyboardState *kstate =
    NEW_NATIVE(ObjKeyboardState, &descriptorKeyboardState);
  kstate->state = SDL_GetKeyboardState(NULL);
  *out = OBJ_VAL(kstate);
  return UTRUE;
}

static CFunction funcGetKeyboardState = {
  implGetKeyboardState, "getKeyboardState", 0 };

/**********************************************************
 * functions: Video
 *********************************************************/

static ubool implCreateRenderer(i16 argCount, Value *args, Value *out) {
  ObjRenderer *renderer;
  ObjWindow *window;
  int index;
  Uint32 flags;

  window = (ObjWindow*)AS_OBJ(args[0]);
  index = (int)AS_NUMBER(args[1]);
  flags = (Uint32)AS_NUMBER(args[2]);

  renderer = NEW_NATIVE(ObjRenderer, &descriptorRenderer);
  renderer->handle = SDL_CreateRenderer(window->handle, index, flags);
  if (renderer->handle == NULL) {
    runtimeError("Failed to create SDL renderer: %s", SDL_GetError());
    return UFALSE;
  }

  *out = OBJ_VAL(renderer);
  return UTRUE;
}

static TypePattern argsCreateRenderer[] = {
  { TYPE_PATTERN_NATIVE, &descriptorWindow },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcCreateRenderer = {
  implCreateRenderer, "createRenderer",
  sizeof(argsCreateRenderer)/sizeof(TypePattern), 0,
  argsCreateRenderer };

/**********************************************************
 * All the functions
 *********************************************************/

static CFunction *functions[] = {
  &funcInit,
  &funcQuit,
  &funcCreateWindow,
  &funcPollEvent,
  &funcGetKeyboardState,
  &funcCreateRenderer,
};

#endif/*mtots_m_sdl_funcs_h*/
