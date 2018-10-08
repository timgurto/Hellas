#include <SDL_image.h>
#include <map>

#include "../../../src/XmlReader.h"
#include "../../../src/XmlWriter.h"
#include "../../../src/client/Renderer.h"
#include "../../../src/client/ui/ChoiceList.h"

#include "Map.h"
#include "Terrain.h"
#include "main.h"

extern Renderer renderer;
extern TerrainType::Container terrain;
extern ChoiceList *terrainList;

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
    if (y >= _dimY) break;
    auto rowTerrain = std::string{};
    xr.findAttr(row, "terrain", rowTerrain);
    auto xToRead = min<int>(
        _dimX, rowTerrain.size());  // Prevent buffer overflow if size changed
    for (auto x = 0; x != xToRead; ++x) {
      _tiles[x][y] = rowTerrain[x];
    }
  }

  generateTexture();
}

void Map::save(const std::string &filename, MapPoint playerSpawn,
               int playerSpawnRange) {
  auto xw = XmlWriter{filename};

  auto e = xw.addChild("size");
  xw.setAttr(e, "x", _dimX);
  xw.setAttr(e, "y", _dimY);

  e = xw.addChild("newPlayerSpawn");
  xw.setAttr(e, "x", playerSpawn.x);
  xw.setAttr(e, "y", playerSpawn.y);
  xw.setAttr(e, "range", playerSpawnRange);

  for (auto y = 0; y != _dimY; ++y) {
    auto rowString = std::string{};
    for (auto x = 0; x != _dimX; ++x) {
      rowString += (_tiles[x][y]);
    }
    e = xw.addChild("row");
    xw.setAttr(e, "y", y);
    xw.setAttr(e, "terrain", rowString);
  }

  xw.publish();

  saveMapImage();
}

void Map::saveMapImage() {
  const auto EDGE_SIZE = 700_px;
  auto canvas = Texture{EDGE_SIZE, EDGE_SIZE};
  auto filename = "map-" + toString(EDGE_SIZE) + ".png";

  _wholeMap.draw({0, 0, EDGE_SIZE, EDGE_SIZE});

  auto surface = SDL_CreateRGBSurface(0, EDGE_SIZE, EDGE_SIZE, 32, 0, 0, 0, 0);
  auto clip = SDL_Rect{0, 0, EDGE_SIZE, EDGE_SIZE};
  SDL_RenderReadPixels(renderer.raw(), &clip, surface->format->format,
                       surface->pixels, surface->pitch);
  IMG_SavePNG(surface, filename.c_str());
  SDL_FreeSurface(surface);
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
      drawTile(x, y);
    }

  renderer.present();

  renderer.popRenderTarget();
}

void Map::drawTile(int x, int y) {
  auto terrainHere = _tiles[x][y];
  renderer.setDrawColor(terrain[terrainHere].color);

  auto rect = ScreenRect{x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
  if (y % 2 == 1) rect.x -= TILE_SIZE / 2;
  renderer.fillRect(rect);
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

void Map::set(int x, int y) {
  if (x < 0 || y < 0) return;

  auto terrainType = terrainList->getSelected();
  if (terrainType.empty()) return;
  auto id = terrainType[0];

  _tiles[x][y] = id;

  renderer.pushRenderTarget(_wholeMap);
  drawTile(x, y);
  renderer.present();
  renderer.popRenderTarget();
}

char Map::at(int x, int y) {
  if (x < 0 || x >= _dimX) return 'a';
  if (y < 0 || y >= _dimY) return 'a';
  return _tiles[x][y];
}
