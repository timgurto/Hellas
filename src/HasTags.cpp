#include "HasTags.h"

#include "XmlReader.h"
#include "util.h"

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

bool HasTags::hasTag(const std::string& tagName) const {
  return _tags.find(tagName) != _tags.end();
}

double HasTags::toolSpeed(const std::string& tag) const {
  auto it = _tags.find(tag);
  if (it == _tags.end()) return 1.0;
  return it->second;
}

#ifdef CLIENT
std::string HasTags::toolSpeedDisplayText(const std::string& tag) const {
  auto speed = toolSpeed(tag);
  if (speed == 1.0) return {};

  auto oss = std::ostringstream{};
  auto displayVal = toInt((speed - 1.0) * 100);
  if (displayVal > 0) oss << " +";
  oss << displayVal << "%";
  return oss.str();
}
#endif
