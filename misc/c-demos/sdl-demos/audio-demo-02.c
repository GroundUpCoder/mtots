/* Audio demo using the QueueAudio API */

#define SDL_MAIN_HANDLED
#include <SDL.h>

#define AMPLITUDE 63
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE (SAMPLE_RATE * 3)

int main(int argc, char* argv[]) {
  SDL_AudioSpec spec;
  SDL_AudioDeviceID dev;
  Sint8 buffer[SAMPLE_SIZE];
  size_t i;

  for (i = 0; i < SAMPLE_SIZE; i++) {
    double time = i / (double) SAMPLE_RATE;
    buffer[i] = (Sint8) (AMPLITUDE * sin(
      2.0f * M_PI * 441.0f * time));
  }

  /* Initialize SDL. */
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_AUDIO) < 0) return 1;

  SDL_memset(&spec, 0, sizeof(spec));
  spec.freq = SAMPLE_RATE;
  spec.format = AUDIO_S8;
  spec.channels = 1;
  spec.samples = 4096;
  spec.callback = NULL;

  dev = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
  if (dev == 0) {
    fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
    return 1;
  }

  if (SDL_QueueAudio(dev, buffer, SAMPLE_SIZE) != 0) {
    fprintf(stderr, "Failed to queue audio: %s\n", SDL_GetError());
    return 1;
  }

  SDL_PauseAudioDevice(dev, 0);
  SDL_Delay(3000);
  return 0;
}
