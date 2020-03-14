#include "SpriteType.h"

#include <SDL.h>
#include <SDL_image.h>

#include "../Color.h"
#include "Client.h"
#include "Surface.h"

ms_t SpriteType::timeThatTheLastRedrawWasOrdered{0};

const double SpriteType::SHADOW_RATIO = 0.8;
const double SpriteType::SHADOW_WIDTH_HEIGHT_RATIO = 1.8;

SpriteType::SpriteType(const ScreenRect &drawRect, const std::string &imageFile)
    : _drawRect(drawRect), _isFlat(false), _isDecoration(false) {
  if (imageFile.empty()) return;
  _image = {imageFile, Color::MAGENTA};
  if (_image) {
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
  }
  setHighlightImage(imageFile);
}

SpriteType::SpriteType(Special special) : _isFlat(false) {
  switch (special) {
    case DECORATION:
      _isDecoration = true;
      break;
  }
}

void SpriteType::addParticles(const std::string &profileName,
                              const MapPoint &offset) {
  Particles p;
  p.profile = profileName;
  p.offset = offset;
  _particles.push_back(p);
}

void SpriteType::setHighlightImage(const std::string &imageFile) {
  Surface highlightSurface(imageFile, Color::MAGENTA);
  if (!highlightSurface) return;
  highlightSurface.swapAllVisibleColors(Color::SPRITE_OUTLINE_HIGHLIGHT);
  auto recolor = Texture{highlightSurface};
  recolor.setAlpha(0x9f);

  _imageHighlight = Texture{recolor.width() + 2, recolor.height() + 2};
  _imageHighlight.setBlend();
  renderer.pushRenderTarget(_imageHighlight);
  for (auto x = 0; x <= 2; ++x)
    for (auto y = 0; y <= 2; ++y) {
      if (x == 1 && y == 1) continue;
      recolor.draw(x, y);
    }
  _image.draw(1, 1);
  renderer.popRenderTarget();
}

void SpriteType::setImage(const std::string &imageFile) {
  _image = Texture(imageFile, Color::MAGENTA);
  _drawRect.w = _image.width();
  _drawRect.h = _image.height();
  setHighlightImage(imageFile);
}

void SpriteType::drawRect(const ScreenRect &rect) { _drawRect = rect; }

const Texture &SpriteType::shadow() const {
  auto shadowIsUpToDate =
      _shadow && _timeGenerated >= timeThatTheLastRedrawWasOrdered;
  if (shadowIsUpToDate) return _shadow;

  px_t shadowWidth = toInt(_drawRect.w * SHADOW_RATIO);
  if (_customShadowWidth.hasValue()) shadowWidth = _customShadowWidth.value();
  px_t shadowHeight = toInt(shadowWidth / SHADOW_WIDTH_HEIGHT_RATIO);
  _shadow = {shadowWidth, shadowHeight};
  _shadow.setBlend();
  _shadow.setAlpha(0x4f);
  renderer.pushRenderTarget(_shadow);
  Client::instance().shadowImage().draw({0, 0, shadowWidth, shadowHeight});
  renderer.popRenderTarget();

  _timeGenerated = SDL_GetTicks();

  return _shadow;
}
