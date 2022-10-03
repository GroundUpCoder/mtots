#ifndef mtots_m_sdl_funcs_h
#define mtots_m_sdl_funcs_h

#include "mtots_m_sdl_common.h"
#include "mtots_m_sdl_acb.h"

/**********************************************************
 * functions: Initialization and Startup
 *********************************************************/

static ubool implInit(i16 argCount, Value *args, Value *out) {
  Uint32 flags = SDL_INIT_VIDEO;
  if (argCount > 0) {
    flags = (Uint32) AS_NUMBER(args[0]);
  }
  SDL_SetMainReady();
  if (SDL_Init(flags) < 0) {
    runtimeError("Failed to init SDL video: %s", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static TypePattern argsInit[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcInit = {
  implInit, "init",
  0, sizeof(argsInit)/sizeof(TypePattern),
  argsInit };

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
    (int) AS_U32(args[1]),
    (int) AS_U32(args[2]),
    (int) AS_U32(args[3]),
    (int) AS_U32(args[4]),
    AS_U32(args[5]));
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
 * functions: Timer
 *********************************************************/

static ubool implGetTicks(i16 argCount, Value *args, Value *out) {
  /* This is a bit naughty, but since we're going to be returning a
   * double anyway, I thnk it's worth just using GetPerformanceCounter()
   * instead of GetTicks */

  /* *out = NUMBER_VAL(SDL_GetTicks64()); */

  *out = NUMBER_VAL(
    SDL_GetPerformanceCounter()
    / (double) SDL_GetPerformanceFrequency()
    * 1000);

  return UTRUE;
}

static CFunction funcGetTicks = { implGetTicks, "getTicks", 0 };

static ubool implDelay(i16 argCount, Value *args, Value *out) {
  SDL_Delay(AS_NUMBER(args[0]));
  return UTRUE;
}

static TypePattern argsDelay[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcDelay = { implDelay, "delay",
  sizeof(argsDelay)/sizeof(TypePattern), 0, argsDelay };

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

static ubool implGetMouseState(i16 argCount, Value *args, Value *out) {
  ObjPoint *point = (ObjPoint*)AS_OBJ(args[0]);
  Uint32 state = SDL_GetMouseState(&point->handle.x, &point->handle.y);
  *out = NUMBER_VAL(state);
  return UTRUE;
}

static TypePattern argsGetMouseState[] = {
  { TYPE_PATTERN_NATIVE, &descriptorPoint },
};

static CFunction funcGetMouseState = { implGetMouseState, "getMouseState",
  sizeof(argsGetMouseState)/sizeof(TypePattern), 0, argsGetMouseState };

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

static ubool implCreateRGBSurfaceFrom(i16 argCount, Value *args, Value *out) {
  ObjByteArray *byteArray = AS_BYTE_ARRAY(args[0]);
  int width = AS_NUMBER(args[1]);
  int height = AS_NUMBER(args[2]);
  int depth = AS_NUMBER(args[3]);
  int pitch = AS_NUMBER(args[4]);
  Uint32 rmask = AS_NUMBER(args[5]);
  Uint32 gmask = AS_NUMBER(args[6]);
  Uint32 bmask = AS_NUMBER(args[7]);
  Uint32 amask = AS_NUMBER(args[8]);
  ObjSurface *surface = NEW_NATIVE(ObjSurface, &descriptorSurface);
  surface->pixelData = args[0];
  surface->handle = SDL_CreateRGBSurfaceFrom(
    byteArray->buffer,
    width, height, depth, pitch, rmask, gmask, bmask, amask);
  if (surface->handle == NULL) {
    runtimeError("Failed to create SDL Surface: %s", SDL_GetError());
    return UFALSE;
  }
  *out = OBJ_VAL(surface);
  return UTRUE;
}

static TypePattern argsCreateRGBSurfaceFrom[] = {
  { TYPE_PATTERN_BYTE_ARRAY },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcCreateRGBSurfaceFrom = {
  implCreateRGBSurfaceFrom, "createRGBSurfaceFrom",
  sizeof(argsCreateRGBSurfaceFrom)/sizeof(TypePattern), 0,
  argsCreateRGBSurfaceFrom };

/**********************************************************
 * functions: Audio
 *********************************************************/

static ubool implOpenAudioDevice(i16 argCount, Value *args, Value *out) {
  const char *device = IS_NIL(args[0]) ? NULL : AS_STRING(args[0])->chars;
  int isCapture = AS_I32(args[1]);
  const SDL_AudioSpec *want = &((ObjAudioSpec*)AS_OBJ(args[2]))->data;
  SDL_AudioSpec *have =
    IS_NIL(args[3]) ? NULL : &((ObjAudioSpec*)AS_OBJ(args[3]))->data;
  int allowedChanges = AS_I32(args[4]);
  SDL_AudioDeviceID handle = SDL_OpenAudioDevice(
    device, isCapture, want, have, allowedChanges);
  ObjAudioDevice *ad;
  if (handle == 0) {
    runtimeError("Could not open SDL Audio Device: %s", SDL_GetError());
    return UFALSE;
  }
  ad = NEW_NATIVE(ObjAudioDevice, &descriptorAudioDevice);
  ad->handle = handle;
  *out = OBJ_VAL(ad);
  return UTRUE;
}

static TypePattern argsOpenAudioDevice[] = {
  { TYPE_PATTERN_STRING_OR_NIL },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NATIVE, &descriptorAudioSpec },
  { TYPE_PATTERN_NATIVE_OR_NIL, &descriptorAudioSpec },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcOpenAudioDevice = {
  implOpenAudioDevice, "openAudioDevice",
  sizeof(argsOpenAudioDevice)/sizeof(TypePattern), 0,
  argsOpenAudioDevice,
};

static ubool implSetCallbackSpec(i16 argCount, Value *args, Value *out) {
  AudioCallbackSpec spec;
  u32 i = AS_U32(args[0]);
  double frequency = AS_NUMBER(args[1]);
  double amplitude = AS_NUMBER(args[2]);
  if (i < 0) {
    i = 0;
  } else if (i >= AUDIO_CALLBACK_ENTRY_COUNT) {
    i = AUDIO_CALLBACK_ENTRY_COUNT - 1;
  }
  if (SDL_LockMutex(audioCallbackMutex) == 0) {
    spec = audioCallbackSpec;
    spec.entries[i].frequency = frequency;
    spec.entries[i].amplitude = amplitude;
    audioCallbackSpec = spec;
    SDL_UnlockMutex(audioCallbackMutex);
  } else {
    panic("Failed to lock audioCallbackMutex");
  }
  return UTRUE;
}

static TypePattern argsSetCallbackSpec[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcSetCallbackSpec = {
  implSetCallbackSpec, "setCallbackSpec",
  sizeof(argsSetCallbackSpec)/sizeof(TypePattern), 0,
  argsSetCallbackSpec,
};

/**********************************************************
 * All the functions
 *********************************************************/

static CFunction *functions[] = {
  &funcInit,
  &funcQuit,
  &funcCreateWindow,
  &funcGetTicks,
  &funcDelay,
  &funcPollEvent,
  &funcGetMouseState,
  &funcGetKeyboardState,
  &funcCreateRenderer,
  &funcCreateRGBSurfaceFrom,
  &funcOpenAudioDevice,
  &funcSetCallbackSpec,
};

#endif/*mtots_m_sdl_funcs_h*/
