#include "Map.h"

#include <SDL_image.h>

#include <map>
#include <queue>

#include "../../../src/XmlReader.h"
#include "../../../src/XmlWriter.h"
#include "../../../src/client/Renderer.h"
#include "../../../src/client/ui/ChoiceList.h"
#include "Terrain.h"
#include "main.h"

extern Renderer renderer;
extern TerrainType::Container terrain;
extern ChoiceList *terrainList;

EMap::EMap(const std::string &filename, MapPoint &playerSpawn,
           int &playerSpawnRange, MapPoint &postTutorialPlayerSpawn) {
  auto xr = XmlReader::FromFile(filename);
  if (!xr) return;

  auto elem = xr.findChild("newPlayerSpawn");
  xr.findAttr(elem, "x", playerSpawn.x);
  xr.findAttr(elem, "y", playerSpawn.y);
  xr.findAttr(elem, "range", playerSpawnRange);

  elem = xr.findChild("postTutorialSpawn");
  xr.findAttr(elem, "x", postTutorialPlayerSpawn.x);
  xr.findAttr(elem, "y", postTutorialPlayerSpawn.y);

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

void EMap::save(const std::string &filename, MapPoint playerSpawn,
                int playerSpawnRange, const MapPoint &postTutorialPlayerSpawn) {
  auto xw = XmlWriter{filename};

  auto e = xw.addChild("size");
  xw.setAttr(e, "x", _dimX);
  xw.setAttr(e, "y", _dimY);

  e = xw.addChild("newPlayerSpawn");
  xw.setAttr(e, "x", playerSpawn.x);
  xw.setAttr(e, "y", playerSpawn.y);
  xw.setAttr(e, "range", playerSpawnRange);

  e = xw.addChild("postTutorialSpawn");
  xw.setAttr(e, "x", postTutorialPlayerSpawn.x);
  xw.setAttr(e, "y", postTutorialPlayerSpawn.y);

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

void EMap::saveMapImage() {
  auto surface =
      SDL_CreateRGBSurface(0, _textureWidth, _textureHeight, 32, 0, 0, 0, 0);

  for (auto y = 0; y != _dimY; ++y)
    for (auto x = 0; x != _dimX; ++x) {
      auto terrainHere = _tiles[x][y];
      auto terrainColorInverted = terrain[terrainHere].color;
      auto color =
          SDL_MapRGB(surface->format, terrainColorInverted.r(),
                     terrainColorInverted.g(), terrainColorInverted.b());

      auto rect = SDL_Rect{x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
      if (y % 2 == 1) rect.x -= TILE_SIZE / 2;
      SDL_FillRect(surface, &rect, color);
    }

  IMG_SavePNG(surface, "../../Images/map.png");

  SDL_FreeSurface(surface);
}

void EMap::generateTexture() {
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

void EMap::drawTile(int x, int y) {
  auto terrainHere = _tiles[x][y];
  renderer.setDrawColor(terrain[terrainHere].color);

  auto rect = ScreenRect{x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
  if (y % 2 == 1) rect.x -= TILE_SIZE / 2;
  renderer.fillRect(rect);
}

void EMap::draw(std::pair<int, int> offset) {
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

bool EMap::isOutOfBounds(int x, int y) const {
  if (x < 0) return true;
  if (y < 0) return true;
  if (x >= _dimX) return true;
  if (y >= _dimY) return true;
  return false;
}

void EMap::set(int x, int y) {
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

struct Tile {
  int x;
  int y;
};

void EMap::fill(int x, int y) {
  renderer.pushRenderTarget(_wholeMap);
  auto terrainType = terrainList->getSelected();
  if (terrainType.empty()) return;
  auto id = terrainType[0];

  auto startingTerrain = at(x, y);
  auto tilesToFill = std::queue<Tile>{};
  tilesToFill.push({x, y});

  const auto MAX_TILES_TO_FILL = 500;

  auto tilesFilled = 0;
  while (!tilesToFill.empty() && tilesFilled++ < MAX_TILES_TO_FILL) {
    // Get tile at the head of the queue
    auto currentTile = tilesToFill.front();
    x = currentTile.x;
    y = currentTile.y;

    // Set this tile
    _tiles[x][y] = id;
    drawTile(x, y);

    // Push all adjacent tiles of the same starting terrain
    auto tilesToConsider = std::vector<Tile>{};
    tilesToConsider.push_back({x - 1, y});
    tilesToConsider.push_back({x + 1, y});
    tilesToConsider.push_back({x, y - 1});
    tilesToConsider.push_back({x, y + 1});
    if (y % 2 == 1) {
      tilesToConsider.push_back({x - 1, y - 1});
      tilesToConsider.push_back({x - 1, y + 1});
    } else {
      tilesToConsider.push_back({x + 1, y - 1});
      tilesToConsider.push_back({x + 1, y + 1});
    }

    for (auto tile : tilesToConsider) {
      if (isOutOfBounds(x, y)) continue;
      if (at(tile.x, tile.y) != startingTerrain) continue;
      tilesToFill.push(tile);
    }

    // Pop
    tilesToFill.pop();
  }

  renderer.present();
  renderer.popRenderTarget();
}

char EMap::at(int x, int y) {
  if (x < 0 || x >= _dimX) return 'a';
  if (y < 0 || y >= _dimY) return 'a';
  return _tiles[x][y];
}
