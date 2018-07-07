#pragma once

#include <memory>
#include <string>
#include <vector>

class Map {
 public:
  Map() {}
  Map(const std::string &filename);

  int width() const { return _dimX * TILE_SIZE; }
  int height() const { return _dimY * TILE_SIZE; }

  SDL_Texture *wholeMap() const { return _wholeMap.get(); }

  void generateTexture();

 private:
  size_t _dimX{0}, _dimY{0};

  static const int TILE_SIZE = 2;

  using Tiles = std::vector<std::vector<char>>;
  mutable Tiles _tiles;

  std::shared_ptr<SDL_Texture> _wholeMap;
};
