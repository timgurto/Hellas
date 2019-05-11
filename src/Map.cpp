#include "Map.h"

Map::Map(size_t width, size_t height) : _w(width), _h(height), _grid(width) {
  for (auto &col : _grid) {
    col = std::vector<char>(height, '\0');
  }
}
