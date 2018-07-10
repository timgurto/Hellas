#include "../../../src/XmlReader.h"

#include "SpawnPoint.h"

void SpawnPoint::load(Container& container, const std::string& filename) {
  auto xr = XmlReader::FromFile(filename);

  for (auto elem : xr.getChildren("spawnPoint")) {
    auto sp = SpawnPoint{};
    sp.type = OBJECT;

    xr.findAttr(elem, "type", sp.id);
    xr.findAttr(elem, "quantity", sp.quantity);
    xr.findAttr(elem, "radius", sp.radius);
    xr.findAttr(elem, "respawnTime", sp.respawnTime);
    xr.findAttr(elem, "x", sp.loc.x);
    xr.findAttr(elem, "y", sp.loc.y);

    container.push_back(sp);
  }
}
