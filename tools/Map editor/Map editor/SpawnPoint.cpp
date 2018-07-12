#include "../../../src/XmlReader.h"

#include "SpawnPoint.h"

bool SpawnPoint::operator<(const SpawnPoint& rhs) const {
  if (loc.y != rhs.loc.y) return loc.y < rhs.loc.y;
  if (loc.x != rhs.loc.x) return loc.x < rhs.loc.x;
  if (id != rhs.id) return id < rhs.id;
  if (quantity != rhs.quantity) return quantity < rhs.quantity;
  if (radius != rhs.radius) return radius < rhs.radius;
  if (respawnTime != rhs.respawnTime) return respawnTime < rhs.respawnTime;
  return false;
}

void SpawnPoint::load(Container& container, const std::string& filename) {
  auto xr = XmlReader::FromFile(filename);

  for (auto elem : xr.getChildren("spawnPoint")) {
    auto sp = SpawnPoint{};

    xr.findAttr(elem, "type", sp.id);
    xr.findAttr(elem, "quantity", sp.quantity);
    xr.findAttr(elem, "radius", sp.radius);
    xr.findAttr(elem, "respawnTime", sp.respawnTime);
    xr.findAttr(elem, "x", sp.loc.x);
    xr.findAttr(elem, "y", sp.loc.y);

    container.insert(sp);
  }
}
