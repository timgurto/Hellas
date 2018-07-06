#include <SDL.h>

#include "../../../src/XmlReader.h"

#include "Map.h"
#include "main.h"

static auto loop = true;

#undef main
int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO);

  auto window =
      SDL_CreateWindow("Hellas Editor", 100, 100, 1280, 720, SDL_WINDOW_SHOWN);
  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  auto map = Map::load("../../Data/map.xml");

  while (loop) {
    handleInput();
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
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
