#include "../../../src/XmlReader.h"

#include "EntityType.h"

void EntityType::load(Container& container, const std::string& filename) {
  auto xr = XmlReader::FromFile(filename);

  for (auto elem : xr.getChildren("objectType")) {
    auto et = EntityType{};

    auto id = std::string{};
    xr.findAttr(elem, "id", id);

    et.image = {"../../Images/Objects/" + id + ".png", Color::MAGENTA};
    et.drawRect.w = et.image.width();
    et.drawRect.h = et.image.height();

    xr.findRectChild("collisionRect", elem, et.collisionRect);

    xr.findAttr(elem, "xDrawOffset", et.drawRect.x);
    xr.findAttr(elem, "yDrawOffset", et.drawRect.y);
    container[id] = et;
  }
}
