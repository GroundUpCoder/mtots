#ifndef mtots_m_sdl_acb_h
#define mtots_m_sdl_acb_h

/* Audio Callback */

#include "mtots_m_sdl_common.h"

#include <string.h>
#include <math.h>

#define SAMPLE_RATE                44100
#define MAX_I8_AMP                    64     /* arbitrary, picked at roughly 1/2 max */
#define AUDIO_CALLBACK_ENTRY_COUNT     8

typedef struct AudioCallbackSpecEntry {
  double frequency;   /* in Hz */
  double amplitude;   /* between 0 and 1 */
  WaveForm waveForm;
} AudioCallbackSpecEntry;

typedef struct AudioCallbackSpec {
  AudioCallbackSpecEntry entries[AUDIO_CALLBACK_ENTRY_COUNT];
} AudioCallbackSpec;

static SDL_mutex *audioCallbackMutex;
static AudioCallbackSpec audioCallbackSpec;

static void audioCallback(void *userdata, Uint8 *rawBuffer, int streamLen) {
  static size_t k = 0;
  size_t i, j;
  AudioCallbackSpec spec;
  Sint8 *buffer = (Sint8*)rawBuffer;

  if (SDL_LockMutex(audioCallbackMutex) == 0) {
    spec = audioCallbackSpec;
    SDL_UnlockMutex(audioCallbackMutex);
  } else {
    panic("FAILED TO LOCK audioCallbackMutex");
  }

  for (i = 0; i < AUDIO_CALLBACK_ENTRY_COUNT; i++) {
    printf("RUNNING audioCallback %d freq=%f, amp=%f\n",
      (int) i, spec.entries[i].frequency, spec.entries[i].amplitude);
    if (spec.entries[i].frequency < 0) {
      spec.entries[i].frequency = 0;
    } else if (spec.entries[i].frequency > 20000) {
      spec.entries[i].frequency = 44100 / 2;
    }

    if (spec.entries[i].amplitude < 0) {
      spec.entries[i].amplitude = 0;
    } else if (spec.entries[i].amplitude > 1) {
      spec.entries[i].amplitude = 1;
    }
  }

  memset(buffer, 0, streamLen);

  for (i = 0; i < streamLen; i++, k++) {
    double t = k / (double) SAMPLE_RATE;
    double sum = 0;
    for (j = 0; j < AUDIO_CALLBACK_ENTRY_COUNT; j++) {
      AudioCallbackSpecEntry *e = &spec.entries[j];
      double ratio = fmod(t * e->frequency, 1);
      if (e->amplitude != 0 && e->frequency != 0) {
        switch (e->waveForm) {
          case WAVE_FORM_SINE:
            sum += e->amplitude * sin(2 * M_PI * ratio);
            break;
          case WAVE_FORM_SAWTOOTH:
            sum += e->amplitude * (ratio * 2 - 1);
            break;
          case WAVE_FORM_SQUARE:
            sum += e->amplitude * (ratio < 0.5 ? -1 : 1);
            break;
          case WAVE_FORM_TRIANGLE:
            sum += e->amplitude * (
              ratio < 0.5 ? (ratio * 4 - 1) : ((1 - ratio) * 4 - 1));
            break;
          case WAVE_FORM_COUNT:
            panic("WAVE_FORM_COUNT");
        }
      }
    }
    if (sum < -1) {
      sum = -1;
    } else if (sum > 1) {
      sum = 1;
    }
    buffer[i] = (Sint8) (MAX_I8_AMP * sum);
  }
}

#endif/*mtots_m_sdl_acb_h*/
