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

  /* init constants */
  { "INIT_TIMER", SDL_INIT_TIMER},
  { "INIT_AUDIO", SDL_INIT_AUDIO},
  { "INIT_VIDEO", SDL_INIT_VIDEO}, /* auto-init events */
  { "INIT_JOYSTICK", SDL_INIT_JOYSTICK}, /* auto-init events */
  { "INIT_HAPTIC", SDL_INIT_HAPTIC},
  { "INIT_GAMECONTROLLER", SDL_INIT_GAMECONTROLLER}, /* auto-init JOYSTICK */
  { "INIT_EVENTS", SDL_INIT_EVENTS},
  { "INIT_EVERYTHING", SDL_INIT_EVERYTHING},

  /* audio sample formats */
  { "AUDIO_S8", AUDIO_S8 },
  { "AUDIO_U8", AUDIO_U8 },
  { "AUDIO_S16LSB", AUDIO_S16LSB },
  { "AUDIO_S16MSB", AUDIO_S16MSB },
  { "AUDIO_S16SYS", AUDIO_S16SYS },
  { "AUDIO_S16", AUDIO_S16 },
  { "AUDIO_S16LSB", AUDIO_S16LSB },
  { "AUDIO_U16LSB", AUDIO_U16LSB },
  { "AUDIO_U16MSB", AUDIO_U16MSB },
  { "AUDIO_U16SYS", AUDIO_U16SYS },
  { "AUDIO_U16", AUDIO_U16 },
  { "AUDIO_U16LSB", AUDIO_U16LSB },
  { "AUDIO_S32LSB", AUDIO_S32LSB },
  { "AUDIO_S32MSB", AUDIO_S32MSB },
  { "AUDIO_S32SYS", AUDIO_S32SYS },
  { "AUDIO_S32", AUDIO_S32 },
  { "AUDIO_S32LSB", AUDIO_S32LSB },
  { "AUDIO_F32LSB", AUDIO_F32LSB },
  { "AUDIO_F32MSB", AUDIO_F32MSB },
  { "AUDIO_F32SYS", AUDIO_F32SYS },
  { "AUDIO_F32", AUDIO_F32 },
  { "AUDIO_F32LSB", AUDIO_F32LSB },

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
