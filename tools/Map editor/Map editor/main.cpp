#include <SDL.h>

#include "../../../src/Color.h"
#include "../../../src/XmlReader.h"

#undef main
int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO);

  auto window =
      SDL_CreateWindow("Hellas Editor", 100, 100, 1280, 720, SDL_WINDOW_SHOWN);
  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  auto xr = XmlReader::FromFile("../../Data/map.xml");
  if (!xr) return 0;

  auto loop = true;
  while (loop) {
    auto e = SDL_Event{};
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          loop = false;
          break;
      }
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}
