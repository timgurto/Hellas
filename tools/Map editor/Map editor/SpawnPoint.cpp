#include "SpawnPoint.h"

#include "../../../src/XmlReader.h"
#include "../../../src/XmlWriter.h"

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
    auto n = 0;
    if (xr.findAttr(elem, "useCachedTerrain", n) && n == 1)
      sp.useCachedTerrain = true;

    container.insert(sp);
  }
}

void SpawnPoint::save(const Container& container, const std::string& filename) {
  auto xw = XmlWriter{filename};

  for (const auto& sp : container) {
    auto e = xw.addChild("spawnPoint");
    xw.setAttr(e, "y", sp.loc.y);
    xw.setAttr(e, "x", sp.loc.x);
    xw.setAttr(e, "type", sp.id);
    xw.setAttr(e, "quantity", sp.quantity);
    xw.setAttr(e, "radius", sp.radius);
    xw.setAttr(e, "respawnTime", sp.respawnTime);
    if (sp.useCachedTerrain) xw.setAttr(e, "useCachedTerrain", 1);
  }
  xw.publish();
}
