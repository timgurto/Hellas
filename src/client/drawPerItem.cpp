#include "drawPerItem.h"
#include "../util.h"
#include "Texture.h"

using namespace std::string_literals;

void DrawPerItemInfo::addEntry(std::string imageFile,
                               const ScreenPoint& offset) {
  _entries.push_back({imageFile, offset});
}

void DrawPerItemInfo::drawItems(const ScreenPoint& location,
                                size_t quantity) const {
  const auto itemsToDraw = min(quantity, _entries.size());
  for (auto i = 0; i != itemsToDraw; ++i) {
    const auto& entry = _entries[i];
    auto image =
        Texture{"Images/Storage/"s + entry.imageFile + ".png", Color::MAGENTA};
    image.draw(location + entry.offset);
  }
}
