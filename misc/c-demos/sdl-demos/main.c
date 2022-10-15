#define SDL_MAIN_HANDLED
#include <SDL.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main(int argc, char* argv[]) {
  int quit = 0;
  SDL_Event e;
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Rect fillRect = {
    SCREEN_WIDTH / 4,
    SCREEN_HEIGHT / 4,
    SCREEN_WIDTH / 2,
    SCREEN_HEIGHT / 2 };

  /* Initialize SDL. */
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

  /* Create the window where we will draw. */
  window = SDL_CreateWindow("SDL_RenderClear",
                            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            SCREEN_WIDTH, SCREEN_HEIGHT, 0);

  /* We must call SDL_CreateRenderer in order for draw calls to affect this
   * window. */
  renderer = SDL_CreateRenderer(window, -1, 0);

  /* Select the color for drawing. It is set to red here. */
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xA0, 0xFF, 0xFF);

  /* Clear the entire screen to our selected color. */
  SDL_RenderClear(renderer);

  /* Fill rect with another color */
  SDL_SetRenderDrawColor(renderer, 0xA0, 0xB0, 0x00, 0xFF);
  SDL_RenderFillRect(renderer, &fillRect);

  /* Up until now everything was drawn behind the scenes.
     This will show the new, red contents of the window. */
  SDL_RenderPresent(renderer);

  /* Hack to get window to stay up */
  while (quit == 0){
    while(SDL_PollEvent(&e)){
      if(e.type == SDL_QUIT) quit = 1;
    }
  }

  SDL_DestroyWindow(window);

  /* Always be sure to clean up */
  SDL_Quit();
  return 0;
}
