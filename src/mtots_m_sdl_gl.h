#ifndef mtots_m_sdl_gl_h
#define mtots_m_sdl_gl_h

#include "mtots_m_sdl_common.h"
#include "mtots_vm.h"

#include <stdio.h>

typedef struct SDLGLConstant {
  const char *name;
  double value;
} SDLGLConstant;

static SDLGLConstant sdlglConstants[] = {
  /* SDL_GLattr */
  { "RED_SIZE", SDL_GL_RED_SIZE },
  { "GREEN_SIZE", SDL_GL_GREEN_SIZE },
  { "BLUE_SIZE", SDL_GL_BLUE_SIZE },
  { "ALPHA_SIZE", SDL_GL_ALPHA_SIZE },
  { "BUFFER_SIZE", SDL_GL_BUFFER_SIZE },
  { "DOUBLEBUFFER", SDL_GL_DOUBLEBUFFER },
  { "DEPTH_SIZE", SDL_GL_DEPTH_SIZE },
  { "STENCIL_SIZE", SDL_GL_STENCIL_SIZE },
  { "ACCUM_RED_SIZE", SDL_GL_ACCUM_RED_SIZE },
  { "ACCUM_GREEN_SIZE", SDL_GL_ACCUM_GREEN_SIZE },
  { "ACCUM_BLUE_SIZE", SDL_GL_ACCUM_BLUE_SIZE },
  { "ACCUM_ALPHA_SIZE", SDL_GL_ACCUM_ALPHA_SIZE },
  { "STEREO", SDL_GL_STEREO },
  { "MULTISAMPLEBUFFERS", SDL_GL_MULTISAMPLEBUFFERS },
  { "MULTISAMPLESAMPLES", SDL_GL_MULTISAMPLESAMPLES },
  { "ACCELERATED_VISUAL", SDL_GL_ACCELERATED_VISUAL },
  { "RETAINED_BACKING", SDL_GL_RETAINED_BACKING },
  { "CONTEXT_MAJOR_VERSION", SDL_GL_CONTEXT_MAJOR_VERSION },
  { "CONTEXT_MINOR_VERSION", SDL_GL_CONTEXT_MINOR_VERSION },
  { "CONTEXT_EGL", SDL_GL_CONTEXT_EGL },
  { "CONTEXT_FLAGS", SDL_GL_CONTEXT_FLAGS },
  { "CONTEXT_PROFILE_MASK", SDL_GL_CONTEXT_PROFILE_MASK },
  { "SHARE_WITH_CURRENT_CONTEXT", SDL_GL_SHARE_WITH_CURRENT_CONTEXT },
  { "FRAMEBUFFER_SRGB_CAPABLE", SDL_GL_FRAMEBUFFER_SRGB_CAPABLE },
  { "CONTEXT_RELEASE_BEHAVIOR", SDL_GL_CONTEXT_RELEASE_BEHAVIOR },
  { "CONTEXT_RESET_NOTIFICATION", SDL_GL_CONTEXT_RESET_NOTIFICATION },
  { "CONTEXT_NO_ERROR", SDL_GL_CONTEXT_NO_ERROR },
  { "FLOATBUFFERS", SDL_GL_FLOATBUFFERS },

  /* context */
  { "CONTEXT_PROFILE_CORE", SDL_GL_CONTEXT_PROFILE_CORE },
  { "CONTEXT_PROFILE_COMPATIBILITY", SDL_GL_CONTEXT_PROFILE_COMPATIBILITY },
  { "CONTEXT_PROFILE_ES", SDL_GL_CONTEXT_PROFILE_ES },
  { "CONTEXT_DEBUG_FLAG", SDL_GL_CONTEXT_DEBUG_FLAG },
  { "CONTEXT_FORWARD_COMPATIBLE_FLAG", SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG },
  { "CONTEXT_ROBUST_ACCESS_FLAG", SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG },
  { "CONTEXT_RESET_ISOLATION_FLAG", SDL_GL_CONTEXT_RESET_ISOLATION_FLAG },
  { "CONTEXT_RELEASE_BEHAVIOR_NONE", SDL_GL_CONTEXT_RELEASE_BEHAVIOR_NONE },
  { "CONTEXT_RELEASE_BEHAVIOR_FLUSH", SDL_GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH },
  { "CONTEXT_RESET_NO_NOTIFICATION", SDL_GL_CONTEXT_RESET_NO_NOTIFICATION },
  { "CONTEXT_RESET_LOSE_CONTEXT", SDL_GL_CONTEXT_RESET_LOSE_CONTEXT },
};

ubool implSDLGLCreateContext(i16 argCount, Value *args, Value *out) {
  ObjWindow *window = (ObjWindow*)AS_OBJ(args[0]);
  SDL_GLContext context = SDL_GL_CreateContext(window->handle);

  /* NOTE: Consider adding a `void*` type to mtots so that I can return the exact
   * context value here */
  if (context == NULL) {
    runtimeError("SDL_GL_CreateConext failed: %s", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static TypePattern argsSDLGLCreateContext[] = {
  { TYPE_PATTERN_NATIVE, &descriptorWindow },
};

static CFunction funcSDLGLCreateContext = {
  implSDLGLCreateContext, "createContext", 1, 0, argsSDLGLCreateContext };

ubool implSDLGLGetDrawableSize(i16 argCount, Value *args, Value *out) {
  ObjWindow *window = (ObjWindow*)AS_OBJ(args[0]);
  ObjPoint *point = (ObjPoint*)AS_OBJ(args[1]);
  SDL_GL_GetDrawableSize(window->handle, &point->handle.x, &point->handle.y);
  return UTRUE;
}

static TypePattern argsSDLGLGetDrawableSize[] = {
  { TYPE_PATTERN_NATIVE, &descriptorWindow },
  { TYPE_PATTERN_NATIVE, &descriptorPoint },
};

static CFunction funcSDLGLGetDrawableSize = {
  implSDLGLGetDrawableSize, "getDrawableSize",
  sizeof(argsSDLGLGetDrawableSize)/sizeof(TypePattern), 0,
  argsSDLGLGetDrawableSize };

ubool implSDLGLSetAttribute(i16 argCount, Value *args, Value *out) {
  u32 attr = AS_U32(args[0]);
  i32 value = AS_I32(args[1]);
  if (SDL_GL_SetAttribute(attr, value) != 0) {
    runtimeError("SDL_GL_SetAttribute error: %s\n", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static TypePattern argsSDLGLSetAttribute[] = {
  { TYPE_PATTERN_NUMBER },
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcSDLGLSetAttribute = {
  implSDLGLSetAttribute, "setAttribute", 2, 0, argsSDLGLSetAttribute };

ubool implSDLGLSwapWindow(i16 argCount, Value *args, Value *out) {
  ObjWindow *window = (ObjWindow*)AS_OBJ(args[0]);
  SDL_GL_SwapWindow(window->handle);
  return UTRUE;
}

static TypePattern argsSDLGLSwapWindow[] = {
  { TYPE_PATTERN_NATIVE, &descriptorWindow },
};

static CFunction funcSDLGLSwapWindow = {
  implSDLGLSwapWindow, "swapWindow", 1, 0, argsSDLGLSwapWindow };

ubool implSDLGLSetSwapInterval(i16 argCount, Value *args, Value *out) {
  i32 attr = AS_I32(args[0]);
  if (SDL_GL_SetSwapInterval(attr) != 0) {
    runtimeError("SDL_GL_SetSwapInterval error: %s\n", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static TypePattern argsSDLGLSetSwapInterval[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcSDLGLSetSwapInterval = {
  implSDLGLSetSwapInterval, "setSwapInterval", 1, 0, argsSDLGLSetSwapInterval };

ubool implSDLGLGetSwapInterval(i16 argCount, Value *args, Value *out) {
  printf("STARTING implSDLGLGetSwapInterval\n");
  *out = NUMBER_VAL(SDL_GL_GetSwapInterval());
  return UTRUE;
}

static CFunction funcSDLGLGetSwapInterval = {
  implSDLGLGetSwapInterval, "getSwapInterval" };

static CFunction *sdlglfunctions[] = {
  &funcSDLGLCreateContext,
  &funcSDLGLGetDrawableSize,
  &funcSDLGLSetAttribute,
  &funcSDLGLSwapWindow,
  &funcSDLGLSetSwapInterval,
  &funcSDLGLGetSwapInterval,
};

static ObjInstance *createSDLGLModule() {
  size_t i;
  DictIterator ti;
  DictEntry *entry;
  ObjInstance *module = newModuleFromCString("sdl.gl", UFALSE);
  push(OBJ_VAL(module));

  for (i = 0; i < sizeof(sdlglfunctions)/sizeof(CFunction*); i++) {
    dictSetN(&module->fields, sdlglfunctions[i]->name, CFUNCTION_VAL(sdlglfunctions[i]));
  }

  for (i = 0; i < sizeof(sdlglConstants)/sizeof(SDLGLConstant); i++) {
    SDLGLConstant c = sdlglConstants[i];
    dictSetN(&module->fields, c.name, NUMBER_VAL(c.value));
  }

  initDictIterator(&ti, &module->fields);
  while (dictIteratorNext(&ti, &entry)) {
    dictSet(&module->klass->methods, entry->key, entry->value);
  }

  pop(); /* module */
  return module;
}

#endif/*mtots_m_sdl_gl_h*/
