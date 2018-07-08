#include <map>

#include "../../../src/XmlReader.h"
#include "../../../src/client/Renderer.h"

#include "Map.h"
#include "Terrain.h"
#include "main.h"

extern Renderer renderer;
extern TerrainType::Container terrain;

Map::Map(const std::string &filename, MapPoint &playerSpawn,
         int &playerSpawnRange) {
  auto xr = XmlReader::FromFile(filename);
  if (!xr) return;

  auto elem = xr.findChild("newPlayerSpawn");
  xr.findAttr(elem, "x", playerSpawn.x);
  xr.findAttr(elem, "y", playerSpawn.y);
  xr.findAttr(elem, "range", playerSpawnRange);

  elem = xr.findChild("size");
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
  _wholeMap = {_textureWidth, _textureHeight};

  renderer.pushRenderTarget(_wholeMap);

  renderer.setDrawColor(Color::BLACK);
  renderer.clear();

  for (auto y = 0; y != _dimY; ++y)
    for (auto x = 0; x != _dimX; ++x) {
      auto terrainHere = _tiles[x][y];
      renderer.setDrawColor(terrain[terrainHere].color);

      auto rect =
          ScreenRect{x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
      if (y % 2 == 1) rect.x -= TILE_SIZE / 2;
      renderer.fillRect(rect);
    }

  renderer.present();

  renderer.popRenderTarget();
}

void Map::draw(std::pair<int, int> offset) {
  auto src = ScreenRect{offset.first, offset.second, zoomed(renderer.width()),
                        zoomed(renderer.height())};
  auto dst = ScreenRect{0, 0, renderer.width(), renderer.height()};
  _wholeMap.draw(dst, src);

  auto mapStretched = src.w > _textureWidth || src.h > _textureHeight;
  if (mapStretched) {
    renderer.setDrawColor(Color::RED);
    for (auto i = 0; i != 10; ++i)
      renderer.drawRect(
          {i, i, renderer.width() - 2 * i, renderer.height() - 2 * i});
  }
}
