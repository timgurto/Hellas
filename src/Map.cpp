#include "Map.h"

void Map::loadFromXML(XmlReader &xr) {
  auto elem = xr.findChild("size");
  if (elem == nullptr || !xr.findAttr(elem, "x", _w) ||
      !xr.findAttr(elem, "y", _h)) {
    return;
  }

  _grid = std::vector<std::vector<char> >(_w);
  for (auto &col : _grid) {
    col = std::vector<char>(_h, '\0');
  }

  for (auto row : xr.getChildren("row")) {
    size_t y;
    if (!xr.findAttr(row, "y", y) || y >= _h) break;
    std::string rowTerrain;
    if (!xr.findAttr(row, "terrain", rowTerrain)) break;
    for (size_t x = 0; x != rowTerrain.size(); ++x) {
      if (x > _w) break;
      _grid[x][y] = rowTerrain[x];
    }
  }
}
