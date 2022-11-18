#ifndef mtots_m_sdl_surf_h
#define mtots_m_sdl_surf_h

#include "mtots_m_sdl_common.h"

void blackenSurface(ObjNative *n) {
  ObjSurface *surface = (ObjSurface*)n;
  markValue(surface->pixelData);
}

NativeObjectDescriptor descriptorSurface = {
  blackenSurface, nopFree, NULL, NULL, NULL, sizeof(ObjSurface), "Surface"};

#endif/*mtots_m_sdl_surf_h*/
