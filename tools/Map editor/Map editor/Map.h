#pragma once

#include <string>
#include <vector>

class Map {
 public:
  static Map load(const std::string &filename);

 private:
  size_t _dimX{0}, _dimY{0};

  using Tiles = std::vector<std::vector<char>>;
  Tiles _tiles;
};
