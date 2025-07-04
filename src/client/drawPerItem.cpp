#include "drawPerItem.h"
#include "../util.h"
#include "ClientObject.h"
#include "Sprite.h"
#include "Surface.h"
#include "Texture.h"

using namespace std::string_literals;

extern Renderer renderer;

void DrawPerItemTypeInfo::addEntry(std::string imageFile,
                                   const ScreenPoint& offset) {
  _entries.push_back({imageFile, offset});
}

const DrawPerItemTypeInfo::ItemTextures& DrawPerItemTypeInfo::getTextures(
    std::string imageName) const {
  auto it = _itemTextures.find(imageName);
  if (it != _itemTextures.end()) return it->second;

  auto& textures = _itemTextures[imageName];

  auto surface =
      Surface{"Images/Storage/"s + imageName + ".png", Color::MAGENTA};

  textures.original = Texture{surface};

  surface.swapAllVisibleColors(Color::SPRITE_OUTLINE_HIGHLIGHT);
  textures.highlight = Texture{surface};

  surface.swapAllVisibleColors(Color::SPRITE_OUTLINE);
  textures.outline = Texture{surface};

  return textures;
}

const Texture& DrawPerItemInfo::image() const {
  generateImagesIfNecessary();
  return _image;
}

const Texture& DrawPerItemInfo::highlightImage() const {
  generateImagesIfNecessary();
  return _highlightImage;
}

void DrawPerItemInfo::update(ms_t timeElapsed) {
  if (_timeUntilNextQuantityUpdate == 0) return;

  if (timeElapsed >= _timeUntilNextQuantityUpdate)
    _timeUntilNextQuantityUpdate = 0;
  else
    _timeUntilNextQuantityUpdate -= timeElapsed;
}

void DrawPerItemInfo::generateImagesIfNecessary() const {
  const auto redrawWasOrdered =
      SpriteType::timeLastRedrawWasOrdered() > _timeLastDrawn;

  auto quantityToDraw = size_t{};
  if (_owner.userHasAccess())
    quantityToDraw = _owner.numItemsInContainer();
  else
    quantityToDraw = _type.quantityShownToEnemies;
  quantityToDraw = min(quantityToDraw, _type._entries.size());

  const auto quantityHasChanged =
      quantityToDraw != _quantityLastDrawn && _timeUntilNextQuantityUpdate == 0;
  if (quantityHasChanged) {
    _timeUntilNextQuantityUpdate = TIME_BETWEEN_QUANTITY_UPDATES;

    // Animate one item at a time
    if (_quantityLastDrawn != 0) {  // Assumption: 0 implies never drawn
      if (quantityToDraw > _quantityLastDrawn)
        quantityToDraw = _quantityLastDrawn + 1;
      else
        quantityToDraw = _quantityLastDrawn - 1;
    }
  }

  if (redrawWasOrdered || quantityHasChanged) {
    // Must be done in this order, as the normal image is used to create the
    // highlight image.
    generateImage(quantityToDraw);
    generateHighlightImage(quantityToDraw);

    _quantityLastDrawn = quantityToDraw;
    _timeLastDrawn = SDL_GetTicks();
  }
}

void DrawPerItemInfo::generateImage(size_t quantity) const {
  const auto& baseImage = _owner.objectType()->image();
  _image = {baseImage.width(), baseImage.height()};
  _image.setBlend();

  renderer.pushRenderTarget(_image);
  renderer.fillWithTransparency();

  // Create border from all items
  const auto borderOffsets =
      std::vector<ScreenPoint>{{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  for (auto i = 0; i != quantity; ++i) {
    const auto& entry = _type._entries[i];
    const auto& outlineTexture = _type.getTextures(entry.imageFile).outline;
    for (const auto borderOffset : borderOffsets)
      outlineTexture.draw(entry.offset - _owner.objectType()->drawRect() +
                          borderOffset);
  }

  // Draw items normally on top
  for (auto i = 0; i != quantity; ++i) {
    const auto& entry = _type._entries[i];
    const auto& itemTexture = _type.getTextures(entry.imageFile).original;
    auto texture = Texture{itemTexture};
    texture.draw(entry.offset - _owner.objectType()->drawRect());
  }

  renderer.popRenderTarget();
}

void DrawPerItemInfo::generateHighlightImage(size_t quantity) const {
  const auto& baseImage = _owner.objectType()->image();
  _highlightImage = {baseImage.width() + 2, baseImage.height() + 2};
  _highlightImage.setBlend();

  renderer.pushRenderTarget(_highlightImage);
  renderer.fillWithTransparency();

  // Create highlight from items
  const auto highlightOffsets = std::vector<ScreenPoint>{
      {-2, 0}, {2, 0}, {0, -2}, {0, 2}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
  for (auto i = 0; i != quantity; ++i) {
    const auto& entry = _type._entries[i];
    const auto& highlightTexture = _type.getTextures(entry.imageFile).highlight;
    for (const auto& highlightOffset : highlightOffsets)
      highlightTexture.draw(entry.offset - _owner.objectType()->drawRect() +
                            highlightOffset + ScreenPoint{1, 1});
  }

  // Draw normal image on top
  _image.draw(1, 1);

  renderer.popRenderTarget();
}
