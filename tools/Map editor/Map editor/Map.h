#pragma once

#include <memory>
#include <string>
#include <vector>

class Map {
 public:
  Map() {}
  Map(const std::string &filename);

  size_t width() const { return _dimX; }
  size_t height() const { return _dimY; }

  SDL_Texture *wholeMap() const { return _wholeMap.get(); }

  void generateTexture();

 private:
  size_t _dimX{0}, _dimY{0};

  using Tiles = std::vector<std::vector<char>>;
  mutable Tiles _tiles;

  std::shared_ptr<SDL_Texture> _wholeMap;
};
