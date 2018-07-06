#include <SDL.h>
#include <map>

#include "../../../src/XmlReader.h"

#include "Map.h"
#include "main.h"

auto loop = true;
SDL_Window *window{nullptr};
SDL_Renderer *renderer{nullptr};
auto map = Map{};

#undef main
int main(int argc, char *argv[]) {
  initialiseSDL();

  map = Map::load("../../Data/map.xml");

  render();

  while (loop) {
    handleInput();
  }

  finaliseSDL();
  return 0;
}

void initialiseSDL() {
  SDL_Init(SDL_INIT_VIDEO);

  window = SDL_CreateWindow("Hellas Editor", 100, 100, 1280, 720,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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
      case SDL_WINDOWEVENT:
        switch (e.window.event) {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
          case SDL_WINDOWEVENT_RESIZED:
          case SDL_WINDOWEVENT_MAXIMIZED:
          case SDL_WINDOWEVENT_RESTORED:
            render();
            break;
        }
    }
  }
}

void render() {
  int winW, winH;
  SDL_GetRendererOutputSize(renderer, &winW, &winH);

  auto blueHell = Color{24, 82, 161};
  SDL_SetRenderDrawColor(renderer, blueHell.r(), blueHell.g(), blueHell.b(),
                         255);
  SDL_RenderClear(renderer);

  auto terrainColor = std::map<char, Color>{};

  auto maxX = min<int>(map.width(), winW);
  auto maxY = min<int>(map.height(), winH);

  for (auto y = 0; y != maxY; ++y)
    for (auto x = 0; x != maxX; ++x) {
      auto terrainHere = map.tileAt(x, y);
      auto colorIt = terrainColor.find(terrainHere);
      if (colorIt == terrainColor.end())
        terrainColor[terrainHere] = randomColor();

      auto color = terrainColor[terrainHere];
      auto rect = SDL_Rect{x, y, 1, 1};

      SDL_SetRenderDrawColor(renderer, color.r(), color.g(), color.b(), 0xff);
      SDL_RenderFillRect(renderer, &rect);
    }

  SDL_RenderPresent(renderer);
}

Color randomColor() {
  return {static_cast<Uint8>(rand() % 0x100),
          static_cast<Uint8>(rand() % 0x100),
          static_cast<Uint8>(rand() % 0x100)};
}
