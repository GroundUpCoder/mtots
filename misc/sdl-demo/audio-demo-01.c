/* Audio demo using the callback API */

#define SDL_MAIN_HANDLED
#include <SDL.h>

#define AMPLITUDE 63
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE 4096

typedef struct {
  size_t sampleIndex;
} AudioData;

/* 441 Hz sine wave */
void callback(void *userData, Uint8 *rawBuffer, int len) {
  Sint8 *buffer = (Sint8*)rawBuffer;
  AudioData *ad = (AudioData*)userData;
  size_t i;

  printf("STARTING CALLBACK (len = %d)\n", len);

  for (i = 0; i < len; i++, ad->sampleIndex++) {
    double time = ad->sampleIndex / (double) SAMPLE_RATE;
    buffer[i] = (Sint8) (AMPLITUDE * sin(
      2.0f * M_PI * 441.0f * time));
  }
}

int main(int argc, char* argv[]) {
  SDL_AudioSpec spec;
  SDL_AudioDeviceID dev;
  AudioData ad;

  /* Initialize SDL. */
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_AUDIO) < 0) return 1;

  SDL_memset(&spec, 0, sizeof(spec));
  spec.freq = SAMPLE_RATE;
  spec.format = AUDIO_S8;
  spec.channels = 1;
  spec.samples = 4096;
  spec.callback = callback;
  spec.userdata = &ad;

  dev = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
  if (dev == 0) {
    fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
    return 1;
  }

  SDL_PauseAudioDevice(dev, 0);
  SDL_Delay(3000);
  return 0;
}
