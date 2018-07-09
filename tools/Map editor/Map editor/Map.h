#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../../../src/client/Texture.h"

class Map {
 public:
  Map() {}
  Map(const std::string &filename, MapPoint &playerSpawn,
      int &playerSpawnRange);

  void save(const std::string &filename, MapPoint playerSpawn,
            int playerSpawnRange);

  int width() const { return _textureWidth; }
  int height() const { return _textureHeight; }

  const Texture &wholeMap() const { return _wholeMap; }

  void generateTexture();

  void draw(std::pair<int, int> offset);

 private:
  size_t _dimX{0}, _dimY{0};
  int _textureWidth{0}, _textureHeight{0};

  static const int TILE_SIZE = 2;

  using Tiles = std::vector<std::vector<char>>;
  mutable Tiles _tiles;

  Texture _wholeMap;
};
