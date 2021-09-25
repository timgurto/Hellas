#pragma once

#include <string>
#include <vector>

#include "../Point.h"
#include "Texture.h"

class ClientObject;

class DrawPerItemTypeInfo {
 public:
  void addEntry(std::string imageFile, const ScreenPoint& offset);

  struct Entry {
    std::string imageFile;
    ScreenPoint offset;
  };
  std::vector<Entry> _entries;
};

class DrawPerItemInfo {
 public:
  DrawPerItemInfo(const ClientObject& owner, const DrawPerItemTypeInfo& type)
      : _owner(owner), _type(type) {}

  operator bool() const { return !_type._entries.empty(); }

  const Texture& image() const;

 private:
  const ClientObject& _owner;
  const DrawPerItemTypeInfo& _type;
  mutable Texture _image;
};
