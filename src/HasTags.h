#pragma once
#include <map>

class XmlReader;
class TiXmlElement;

class HasTags {
 public:
  using Tags = std::map<std::string, double>;  // Name -> tool speed

  void loadTagsFromXML(XmlReader &xr, TiXmlElement *elem);
  void addTag(const std::string &tagName, double toolSpeed);
  bool hasAnyTags() const { return _tags.size() > 0; }
  bool hasTag(const std::string &tagName) const;
  const Tags &tags() const { return _tags; }
  double toolSpeed(const std::string &tag) const;

  std::string toolSpeedDisplayText(const std::string &tag) const;

 private:
  Tags _tags;
};
