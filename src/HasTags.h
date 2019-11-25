#pragma once
#include <map>

class HasTags {
 public:
  using Tags = std::map<std::string, double>;  // Name -> tool speed

  void addTag(const std::string &tagName, double toolSpeed = 1.0);
  bool hasTags() const { return _tags.size() > 0; }
  bool isTag(const std::string &tagName) const;
  const Tags &tags() const { return _tags; }

 private:
  Tags _tags;
};
