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
  size_t quantityShownToEnemies{0};

  friend class DrawPerItemInfo;

 private:
  std::vector<Entry> _entries;
};

class DrawPerItemInfo {
 public:
  DrawPerItemInfo(const ClientObject& owner, const DrawPerItemTypeInfo& type)
      : _owner(owner), _type(type) {}

  operator bool() const { return !_type._entries.empty(); }

  const Texture& image() const;
  const Texture& highlightImage() const;

  void update(ms_t timeElapsed);

 private:
  const ClientObject& _owner;
  const DrawPerItemTypeInfo& _type;
  mutable Texture _image, _highlightImage;

  mutable ms_t _timeUntilNextQuantityUpdate{0};
  static const ms_t TIME_BETWEEN_QUANTITY_UPDATES{100};

  mutable size_t _quantityLastDrawn{0};
  mutable ms_t _timeLastDrawn{0};

  void generateImagesIfNecessary() const;
  void generateImage(size_t quantity) const;
  void generateHighlightImage(size_t quantity) const;
};
