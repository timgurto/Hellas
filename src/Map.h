#pragma once

#include <vector>

class Map {
 public:
  Map() {}
  Map(size_t width, size_t height);

  // De-jure sizes
  size_t width() const { return _w; }
  size_t height() const { return _h; }

  // De-facto size.  Used only for verification.
  size_t cols() const { return _grid.size(); }

  const std::vector<char>& operator[](size_t x) const { return _grid[x]; }
  std::vector<char>& operator[](size_t x) { return _grid[x]; }

 private:
  std::vector<std::vector<char> > _grid;
  size_t _w{0}, _h{0};
};
