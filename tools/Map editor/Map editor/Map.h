#pragma once

#include <string>
#include <vector>

class Map {
 public:
  static Map load(const std::string &filename);

  size_t width() const { return _dimX; }
  size_t height() const { return _dimY; }
  char tileAt(size_t x, size_t y) const { return _tiles[x][y]; }

 private:
  size_t _dimX{0}, _dimY{0};

  using Tiles = std::vector<std::vector<char>>;
  mutable Tiles _tiles;
};
