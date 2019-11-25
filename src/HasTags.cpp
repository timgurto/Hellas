#include "HasTags.h"

void HasTags::addTag(const std::string& tagName, double toolSpeed) {
  _tags[tagName] = toolSpeed;
}

bool HasTags::isTag(const std::string& tagName) const {
  return _tags.find(tagName) != _tags.end();
}
