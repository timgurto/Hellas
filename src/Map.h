#pragma once

#include <vector>

#include "XmlReader.h"

class Map {
 public:
  void loadFromXML(XmlReader& xr);

  size_t width() const { return _w; }
  size_t height() const { return _h; }

  const std::vector<char>& operator[](size_t x) const { return _grid[x]; }
  std::vector<char>& operator[](size_t x) { return _grid[x]; }

 private:
  std::vector<std::vector<char> > _grid;
  size_t _w{0}, _h{0};
};
