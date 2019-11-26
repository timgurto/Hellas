#include "HasTags.h"

#include "XmlReader.h"

using namespace std::string_literals;

void HasTags::loadTagsFromXML(XmlReader& xr, TiXmlElement* elem) {
  for (auto child : xr.getChildren("tag", elem)) {
    auto name = ""s;
    if (!xr.findAttr(child, "name", name)) continue;
    auto toolSpeed = 1.0;
    xr.findAttr(child, "toolSpeed", toolSpeed);

    addTag(name, toolSpeed);
  }
}

void HasTags::addTag(const std::string& tagName, double toolSpeed) {
  _tags[tagName] = toolSpeed;
}

bool HasTags::isTag(const std::string& tagName) const {
  return _tags.find(tagName) != _tags.end();
}
