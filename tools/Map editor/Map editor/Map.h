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
  void saveMapImage();

  int width() const { return _textureWidth; }
  int height() const { return _textureHeight; }

  const Texture &wholeMap() const { return _wholeMap; }

  void set(int x, int y);
  void fill(int x, int y);
  char at(int x, int y);

  void generateTexture();
  void drawTile(int x, int y);

  void draw(std::pair<int, int> offset);

 private:
  int _dimX{0}, _dimY{0};
  bool isOutOfBounds(int x, int y) const;
  int _textureWidth{0}, _textureHeight{0};

  static const int TILE_SIZE = 2;

  using Tiles = std::vector<std::vector<char>>;
  mutable Tiles _tiles;

  Texture _wholeMap;
};
