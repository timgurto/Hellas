#include "SpriteType.h"

#include "../Color.h"
#include "Client.h"
#include "Surface.h"

extern Renderer renderer;

ms_t SpriteType::timeThatTheLastRedrawWasOrdered{0};

const double SpriteType::SHADOW_RATIO = 1.1;
const double SpriteType::SHADOW_WIDTH_HEIGHT_RATIO = 1.8;

SpriteType::SpriteType(const ScreenRect &drawRect, const std::string &imageFile)
    : _image(imageFile), _drawRect(drawRect) {
  if (!_image) return;
  _drawRect.w = _image.width();
  _drawRect.h = _image.height();
}

SpriteType SpriteType::DecorationWithNoData() {
  auto ret = SpriteType{};
  ret._isDecoration = true;
  return ret;
}

void SpriteType::addParticles(const std::string &profileName,
                              const MapPoint &offset) {
  Particles p;
  p.profile = profileName;
  p.offset = offset;
  _particles.push_back(p);
}

void SpriteType::setImage(const std::string &imageFile) {
  _image = {imageFile};
  _drawRect.w = _image.width();
  _drawRect.h = _image.height();
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
  Client::images.shadow.draw({0, 0, shadowWidth, shadowHeight});
  renderer.popRenderTarget();

  _timeShadowGenerated = SDL_GetTicks();

  return _shadow;
}
