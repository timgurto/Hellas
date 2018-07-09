#include "../../../src/XmlReader.h"

#include "Terrain.h"

void TerrainType::load(Container &container, const std::string &filename) {
  auto xr = XmlReader::FromFile(filename);

  for (auto elem : xr.getChildren("terrain")) {
    auto t = TerrainType{};

    xr.findAttr(elem, "index", t.index);
    xr.findAttr(elem, "id", t.id);
    xr.findAttr(elem, "color", t.color);

    container[t.index] = t;
  }
}
