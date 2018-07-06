#include <SDL.h>

#include "../../../src/XmlReader.h"

#include "Map.h"
#include "main.h"

static auto loop = true;
SDL_Window *window{nullptr};
SDL_Renderer *renderer{nullptr};

#undef main
int main(int argc, char *argv[]) {
  initialiseSDL();


  auto map = Map::load("../../Data/map.xml");

  while (loop) {
    handleInput();
  }

  finaliseSDL();
  return 0;
}

void initialiseSDL() {
  SDL_Init(SDL_INIT_VIDEO);

  window =
      SDL_CreateWindow("Hellas Editor", 100, 100, 1280, 720, SDL_WINDOW_SHOWN);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

void finaliseSDL() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
}

void handleInput() {
  auto e = SDL_Event{};
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT:
        loop = false;
        break;
    }
  }
}
