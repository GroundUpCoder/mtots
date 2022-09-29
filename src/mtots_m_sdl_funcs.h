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
  ObjWindow *window;
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

static TypePattern argsCreateWindow[] = {
  { TYPE_PATTERN_STRING },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcCreateWindow = { implCreateWindow, "createWindow",
  sizeof(argsCreateWindow)/sizeof(TypePattern), 0, argsCreateWindow };

/**********************************************************
 * functions: Input
 *********************************************************/

static ubool implPollEvent(i16 argCount, Value *args, Value *out) {
  ObjEvent *event;
  event = (ObjEvent*)AS_OBJ(args[0]);
  *out = BOOL_VAL(SDL_PollEvent(&event->data));
  return UTRUE;
}

static TypePattern argsPollEvent[] = {
  { TYPE_PATTERN_NATIVE, &descriptorEvent },
};

static CFunction funcPollEvent = { implPollEvent, "pollEvent",
  sizeof(argsPollEvent)/sizeof(TypePattern), 0, argsPollEvent};

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
