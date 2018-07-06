#include "../../../src/XmlReader.h"

#include "Map.h"

Map Map::load(const std::string& filename) {
  Map m;

  auto xr = XmlReader::FromFile(filename);
  if (!xr) return {};

  auto elem = xr.findChild("size");
  xr.findAttr(elem, "x", m._dimX);
  xr.findAttr(elem, "y", m._dimY);

  m._tiles = Tiles(m._dimX);
  for (auto x = 0; x != m._dimX; ++x)
    m._tiles[x] = std::vector<char>(m._dimY, 0);
  for (auto row : xr.getChildren("row")) {
    auto y = 0;
    auto rowNumberSpecified = xr.findAttr(row, "y", y);
    auto rowTerrain = std::string{};
    xr.findAttr(row, "terrain", rowTerrain);
    for (auto x = 0; x != rowTerrain.size(); ++x) {
      m._tiles[x][y] = rowTerrain[x];
    }
  }

  return m;
}
