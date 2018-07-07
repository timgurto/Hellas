#include <map>

#include "../../../src/XmlReader.h"

#include "Map.h"
#include "Terrain.h"

extern SDL_Renderer* renderer;
extern TerrainType::Container terrain;

Map::Map(const std::string& filename) {
  auto xr = XmlReader::FromFile(filename);
  if (!xr) return;

  auto elem = xr.findChild("size");
  xr.findAttr(elem, "x", _dimX);
  xr.findAttr(elem, "y", _dimY);

  _tiles = Tiles(_dimX);
  for (auto x = 0; x != _dimX; ++x) _tiles[x] = std::vector<char>(_dimY, 0);
  for (auto row : xr.getChildren("row")) {
    auto y = 0;
    auto rowNumberSpecified = xr.findAttr(row, "y", y);
    auto rowTerrain = std::string{};
    xr.findAttr(row, "terrain", rowTerrain);
    for (auto x = 0; x != rowTerrain.size(); ++x) {
      _tiles[x][y] = rowTerrain[x];
    }
  }

  generateTexture();
}

void Map::generateTexture() {
  _textureWidth = _dimX * TILE_SIZE - TILE_SIZE / 2;
  _textureHeight = _dimY * TILE_SIZE - TILE_SIZE / 2;
  _wholeMap = {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                 SDL_TEXTUREACCESS_TARGET, _textureWidth,
                                 _textureHeight),
               SDL_DestroyTexture};

  auto result = SDL_SetRenderTarget(renderer, _wholeMap.get());

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  for (auto y = 0; y != _dimY; ++y)
    for (auto x = 0; x != _dimX; ++x) {
      auto terrainHere = _tiles[x][y];

      auto color = terrain[terrainHere].color;
      auto rect = SDL_Rect{x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
      if (y % 2 == 1) rect.x -= TILE_SIZE / 2;

      SDL_SetRenderDrawColor(renderer, color.r(), color.g(), color.b(), 0xff);
      SDL_RenderFillRect(renderer, &rect);
    }

  SDL_RenderPresent(renderer);

  result = SDL_SetRenderTarget(renderer, nullptr);
}
