#ifndef mtots_m_sdl_render_h
#define mtots_m_sdl_render_h

#include "mtots_m_sdl_common.h"

/**********************************************************
 * Renderer.setDrawColor()
 *********************************************************/

static ubool implRendererSetDrawColor(i16 argCount, Value *args, Value *out) {
  ObjRenderer *renderer = (ObjRenderer*)AS_OBJ(args[-1]);
  SDL_SetRenderDrawColor(
    renderer->handle,
    AS_NUMBER(args[0]),
    AS_NUMBER(args[1]),
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3]));
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
  if (SDL_RenderFillRect(renderer->handle, &rect->handle) != 0) {
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
  *out = OBJ_VAL(texture);
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
 * -- the descriptor --
 *********************************************************/

static CFunction *rendererMethods[] = {
  &funcRendererSetDrawColor,
  &funcRendererClear,
  &funcRendererFillRect,
  &funcRendererPresent,
  &funcRendererCreateTextureFromSurface,
  NULL,
};

NativeObjectDescriptor descriptorRenderer = {
  nopBlacken, nopFree, NULL, NULL, NULL,
  sizeof(ObjRenderer), "Renderer",
  rendererMethods };

#endif/*mtots_m_sdl_render_h*/
