#ifndef mtots_m_sdl_render_h
#define mtots_m_sdl_render_h

#include "mtots_m_sdl_common.h"

/**********************************************************
 * Renderer.setDrawColor()
 *********************************************************/

static ubool implRendererSetDrawColor(i16 argCount, Value *args, Value *out) {
  ObjRenderer *renderer = (ObjRenderer*)AS_OBJ(args[-1]);
  *out = NUMBER_VAL(SDL_SetRenderDrawColor(
    renderer->handle,
    AS_NUMBER(args[0]),
    AS_NUMBER(args[1]),
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3])));
  return UTRUE;
}

static TypePattern argsRendererSetDrawColor[] = {
    { TYPE_PATTERN_NUMBER },
    { TYPE_PATTERN_NUMBER },
    { TYPE_PATTERN_NUMBER },
    { TYPE_PATTERN_NUMBER },
};

static CFunction funcRendererSetDrawColor = {
  implRendererSetDrawColor, "setDrawColor",
  sizeof(argsRendererSetDrawColor)/sizeof(TypePattern), 0,
  argsRendererSetDrawColor };

/**********************************************************
 * Renderer.clear()
 *********************************************************/

static ubool implRendererClear(i16 argCount, Value *args, Value *out) {
  ObjRenderer *renderer = (ObjRenderer*)AS_OBJ(args[-1]);
  if (SDL_RenderClear(renderer->handle) != 0) {
    runtimeError("SDL error: %s", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static CFunction funcRendererClear = { implRendererClear, "clear", 0 };

/**********************************************************
 * Renderer.fillRect()
 *********************************************************/

static ubool implRendererFillRect(i16 argCount, Value *args, Value *out) {
  ObjRenderer *renderer = (ObjRenderer*)AS_OBJ(args[-1]);
  ObjRect *rect = (ObjRect*)AS_OBJ(args[0]);
  if (SDL_RenderFillRect(renderer->handle, &rect->data) != 0) {
    runtimeError("SDL error: %s", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static TypePattern argsRendererFillRect[] = {
  { TYPE_PATTERN_NATIVE, &descriptorRect },
};

static CFunction funcRendererFillRect = {
  implRendererFillRect, "fillRect",
  sizeof(argsRendererFillRect)/sizeof(TypePattern), 0,
  argsRendererFillRect };

/**********************************************************
 * Renderer.drawRect()
 *********************************************************/

static ubool implRendererDrawRect(i16 argCount, Value *args, Value *out) {
  ObjRenderer *renderer = (ObjRenderer*)AS_OBJ(args[-1]);
  ObjRect *rect = (ObjRect*)AS_OBJ(args[0]);
  if (SDL_RenderDrawRect(renderer->handle, &rect->data) != 0) {
    runtimeError("SDL_RenderDrawRect Failed: %s", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static TypePattern argsRendererDrawRect[] = {
  { TYPE_PATTERN_NATIVE, &descriptorRect },
};

static CFunction funcRendererDrawRect = {
  implRendererDrawRect, "drawRect",
  sizeof(argsRendererDrawRect)/sizeof(TypePattern), 0,
  argsRendererDrawRect };

/**********************************************************
 * Renderer.present()
 *********************************************************/

static ubool implRendererPresent(i16 argCount, Value *args, Value *out) {
  ObjRenderer *renderer = (ObjRenderer*)AS_OBJ(args[-1]);
  SDL_RenderPresent(renderer->handle);
  return UTRUE;
}

static CFunction funcRendererPresent = { implRendererPresent, "present", 0 };

/**********************************************************
 * Renderer.createTextureFromSurface()
 *********************************************************/

static ubool implRendererCreateTextureFromSurface(
    i16 argCount, Value *args, Value *out) {
  ObjRenderer *renderer = (ObjRenderer*)AS_OBJ(args[-1]);
  ObjSurface *surface = (ObjSurface*)AS_OBJ(args[0]);
  ObjTexture *texture = NEW_NATIVE(ObjTexture, &descriptorTexture);
  texture->handle = SDL_CreateTextureFromSurface(
    renderer->handle, surface->handle);
  if (texture->handle == NULL) {
    runtimeError(
      "Failed to create SDL texture from surface: %s",
      SDL_GetError());
    return UFALSE;
  }
  *out = OBJ_VAL_EXPLICIT((Obj*)texture);
  return UTRUE;
}

static TypePattern argsRendererCreateTextureFromSurface[] = {
  { TYPE_PATTERN_NATIVE, &descriptorSurface },
};

static CFunction funcRendererCreateTextureFromSurface = {
  implRendererCreateTextureFromSurface, "createTextureFromSurface",
  sizeof(argsRendererCreateTextureFromSurface)/sizeof(TypePattern), 0,
  argsRendererCreateTextureFromSurface };

/**********************************************************
 * Renderer.copy()
 *********************************************************/

static ubool implRendererCopy(i16 argCount, Value *args, Value *out) {
  ObjRenderer *renderer = (ObjRenderer*)AS_OBJ(args[-1]);
  ObjTexture *texture = (ObjTexture*)AS_OBJ(args[0]);
  ObjRect *srcRect = (ObjRect*)AS_OBJ(args[1]);
  ObjRect *dstRect = (ObjRect*)AS_OBJ(args[2]);
  if (SDL_RenderCopy(
      renderer->handle,
      texture->handle,
      &srcRect->data,
      &dstRect->data) != 0) {
    runtimeError("SDL RenderCopy Failed: %s", SDL_GetError());
    return UFALSE;
  };
  return UTRUE;
}

static TypePattern argsRendererCopy[] = {
  { TYPE_PATTERN_NATIVE, &descriptorTexture },
  { TYPE_PATTERN_NATIVE, &descriptorRect },
  { TYPE_PATTERN_NATIVE, &descriptorRect },
};

static CFunction funcRendererCopy = {
  implRendererCopy, "copy",
  sizeof(argsRendererCopy)/sizeof(TypePattern), 0,
  argsRendererCopy };

/**********************************************************
 * -- the descriptor --
 *********************************************************/

static CFunction *rendererMethods[] = {
  &funcRendererSetDrawColor,
  &funcRendererClear,
  &funcRendererFillRect,
  &funcRendererDrawRect,
  &funcRendererPresent,
  &funcRendererCreateTextureFromSurface,
  &funcRendererCopy,
  NULL,
};

NativeObjectDescriptor descriptorRenderer = {
  nopBlacken, nopFree, NULL, NULL, NULL,
  sizeof(ObjRenderer), "Renderer",
  rendererMethods };

#endif/*mtots_m_sdl_render_h*/
