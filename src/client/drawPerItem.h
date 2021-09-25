#pragma once

#include <string>
#include <vector>

#include "../Point.h"

class DrawPerItemInfo {
 public:
  void addEntry(std::string imageFile, const ScreenPoint& offset);
  void drawItems(const ScreenPoint& location, size_t quantity) const;

  struct Entry {
    std::string imageFile;
    ScreenPoint offset;
  };
  std::vector<Entry> _entries;
};
