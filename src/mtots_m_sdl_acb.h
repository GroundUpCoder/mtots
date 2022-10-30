#ifndef mtots_m_sdl_acb_h
#define mtots_m_sdl_acb_h

/* Audio Callback */

#include "mtots_m_sdl_common.h"

#include <string.h>
#include <math.h>

typedef struct AudioTrack {
  double frequency;   /* in Hz */
  double amplitude;   /* between 0 and 1 */
  WaveForm waveForm;
} AudioTrack;

typedef struct AudioTrackSet {
  AudioTrack tracks[AUDIO_TRACK_COUNT];
} AudioTrackSet;

static SDL_mutex *audioTracksetMutex;
static AudioTrackSet audioTrackSet;

static void audioCallback(void *userdata, Uint8 *rawBuffer, int streamLen) {
  static size_t ticks = 0;
  size_t i;
  AudioTrackSet tset;
  Sint8 *buffer = (Sint8*)rawBuffer;

  if (SDL_LockMutex(audioTracksetMutex) == 0) {
    tset = audioTrackSet;
    SDL_UnlockMutex(audioTracksetMutex);
  } else {
    panic("FAILED TO LOCK audioTracksetMutex");
  }

  for (i = 0; i < AUDIO_TRACK_COUNT; i++) {
    if (tset.tracks[i].frequency < 0) {
      tset.tracks[i].frequency = 0;
    } else if (tset.tracks[i].frequency > 20000) {
      tset.tracks[i].frequency = 44100 / 2;
    }

    if (tset.tracks[i].amplitude < 0) {
      tset.tracks[i].amplitude = 0;
    } else if (tset.tracks[i].amplitude > 1) {
      tset.tracks[i].amplitude = 1;
    }
  }

  for (i = 0; i < streamLen; i++, ticks++) {
    double timeInSec = ticks / (double) AUDIO_SAMPLE_RATE;
    double sum = 0;
    size_t j;
    for (j = 0; j < AUDIO_TRACK_COUNT; j++) {
      AudioTrack *e = &tset.tracks[j];
      double ratio = fmod(timeInSec * e->frequency, 1);
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
    sum = sum < -1 ? -1 : sum > 1 ? 1 : sum;
    buffer[i] = (Sint8) (I8_MAX * sum);
  }
}

#endif/*mtots_m_sdl_acb_h*/
