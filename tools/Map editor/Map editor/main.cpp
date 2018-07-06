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

  auto mapX = 0, mapY = 0;
  auto elem = xr.findChild("size");
  xr.findAttr(elem, "x", mapX);
  xr.findAttr(elem, "y", mapY);

  auto map = std::vector<std::vector<char>>(mapX);
  for (auto x = 0; x != mapX; ++x) map[x] = std::vector<char>(mapY, 0);
  for (auto row : xr.getChildren("row")) {
    size_t y;
    auto rowNumberSpecified = xr.findAttr(row, "y", y);
    std::string rowTerrain;
    xr.findAttr(row, "terrain", rowTerrain);
    for (size_t x = 0; x != rowTerrain.size(); ++x) {
      map[x][y] = rowTerrain[x];
    }
  }

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
