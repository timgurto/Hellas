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
    : _imageFile(imageFile),
      _drawRect(drawRect),
      _isFlat(false),
      _isDecoration(false) {
  if (imageFile.empty()) return;
  _image = {imageFile, Color::MAGENTA};
  if (_image) {
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
  }
  setHighlightImage();
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

Texture SpriteType::createHighlightImageFrom(
    const Texture &original, const std::string &originalImageFile) {
  auto surface = Surface{originalImageFile, Color::MAGENTA};
  if (!surface) return {};

  const Uint32 ALPHA_FRIENDLY_HIGHLIGHT_COLOUR =
      Uint32{Color::SPRITE_OUTLINE_HIGHLIGHT} | 0xff000000;

  surface.swapAllVisibleColors(ALPHA_FRIENDLY_HIGHLIGHT_COLOUR);
  auto singleRecolor = Texture{surface};
  singleRecolor.setAlpha(0x9f);

  auto ret = Texture{original.width() + 2, original.height() + 2};
  ret.setBlend();
  renderer.pushRenderTarget(ret);
  for (auto x = 0; x <= 2; ++x)
    for (auto y = 0; y <= 2; ++y) {
      if (x == 1 && y == 1) continue;
      singleRecolor.draw(x, y);
    }
  original.draw(1, 1);
  renderer.popRenderTarget();

  return ret;
}

void SpriteType::setHighlightImage() const {
  _imageHighlight = createHighlightImageFrom(_image, _imageFile);
  _timeHighlightGenerated = SDL_GetTicks();
}

void SpriteType::setImage(const std::string &imageFile) {
  _imageFile = imageFile + ".png";
  _image = Texture(_imageFile, Color::MAGENTA);
  _drawRect.w = _image.width();
  _drawRect.h = _image.height();
  setHighlightImage();
}

const Texture &SpriteType::highlightImage() const {
  auto highlightIsUpToDate =
      _timeHighlightGenerated >= timeThatTheLastRedrawWasOrdered;
  if (!highlightIsUpToDate) setHighlightImage();

  return _imageHighlight;
}

void SpriteType::drawRect(const ScreenRect &rect) { _drawRect = rect; }

const Texture &SpriteType::shadow() const {
  auto shadowIsUpToDate =
      _shadow && _timeShadowGenerated >= timeThatTheLastRedrawWasOrdered;
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

  _timeShadowGenerated = SDL_GetTicks();

  return _shadow;
}
