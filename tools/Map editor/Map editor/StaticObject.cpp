#include "../../../src/XmlReader.h"

#include "StaticObject.h"

void StaticObject::load(Container& container, const std::string& filename) {
  auto xr = XmlReader::FromFile(filename);

  for (auto elem : xr.getChildren("object")) {
    auto so = StaticObject{};

    xr.findAttr(elem, "id", so.id);
    xr.findAttr(elem, "x", so.loc.x);
    xr.findAttr(elem, "y", so.loc.y);


    container.push_back(so);
  }

  for (auto elem : xr.getChildren("npc")) {
    auto so = StaticObject{};

    xr.findAttr(elem, "id", so.id);

    auto location = xr.findChild("location", elem);
    xr.findAttr(location, "x", so.loc.x);
    xr.findAttr(location, "y", so.loc.y);

    container.push_back(so);
  }
}
