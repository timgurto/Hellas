#include "drawPerItem.h"
#include "../util.h"
#include "ClientObject.h"
#include "Sprite.h"
#include "Texture.h"

using namespace std::string_literals;

extern Renderer renderer;

void DrawPerItemTypeInfo::addEntry(std::string imageFile,
                                   const ScreenPoint& offset) {
  _entries.push_back({imageFile, offset});
}

const Texture& DrawPerItemInfo::image() const {
  const auto& baseImage = _owner.objectType()->image();
  _image = {baseImage.width(), baseImage.height()};
  _image.setBlend();

  renderer.pushRenderTarget(_image);
  renderer.fillWithTransparency();

  // Draw base image
  baseImage.draw();

  // Draw items on top
  const auto itemsToDraw =
      min(_owner.numItemsInContainer(), _type._entries.size());
  for (auto i = 0; i != itemsToDraw; ++i) {
    const auto& entry = _type._entries[i];
    auto itemImage =
        Texture{"Images/Storage/"s + entry.imageFile + ".png", Color::MAGENTA};
    itemImage.draw(entry.offset - _owner.objectType()->drawRect());
  }

  renderer.popRenderTarget();

  return _image;
}
