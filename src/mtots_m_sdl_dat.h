#ifndef mtots_m_sdl_dat_h
#define mtots_m_sdl_dat_h

/* This header is meant to be included only by mtots_m_sdl.c */

#define SDL_MAIN_HANDLED
#include <SDL.h>

typedef struct {
  const char *name;
  double value;
} NumericConstant;

static NumericConstant numericConstants[] = {
  {"WINDOWPOS_CENTERED", SDL_WINDOWPOS_CENTERED},

  /* Event types */
  {"FINGERMOTION", SDL_FINGERMOTION},
  {"FINGERDOWN", SDL_FINGERDOWN},
  {"FINGERUP", SDL_FINGERUP},
  {"KEYDOWN", SDL_KEYDOWN},
  {"KEYUP", SDL_KEYUP},
  {"MOUSEMOTION", SDL_MOUSEMOTION},
  {"MOUSEBUTTONDOWN", SDL_MOUSEBUTTONDOWN},
  {"MOUSEBUTTONUP", SDL_MOUSEBUTTONUP},
  {"MOUSEWHEEL", SDL_MOUSEWHEEL},
  {"QUIT", SDL_QUIT},
};

#endif/*mtots_m_sdl_dat_h*/
