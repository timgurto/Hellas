#include <cassert>

#include "../XmlReader.h"
#include "Client.h"
#include "Tag.h"

const std::string &TagNames::operator[](const std::string &id) const {
  auto it = container_.find(id);
  if (it == container_.end()) return id;
  return it->second;
}

void TagNames::readFromXML(XmlReader &xr) {
  if (!xr) {
    return;
  }
  for (auto elem : xr.getChildren("tag")) {
    std::string id, name;
    if (!xr.findAttr(elem, "id", id) || !xr.findAttr(elem, "name", name)) {
      Client::instance().showErrorMessage(
          "Skipping tag with insufficient info."s, Color::TODO);
      continue;
    }
    container_[id] = name;
  }
}
