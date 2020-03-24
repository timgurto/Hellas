#include "Particle.h"

#include "../util.h"
#include "Client.h"
#include "ParticleProfile.h"

const SpriteType Particle::ENTITY_TYPE = SpriteType::DecorationWithNoData();

Particle::Particle(const MapPoint &loc, const Texture &image,
                   const ScreenRect &drawRect, const MapPoint &velocity,
                   double startingAltitude, double startingFallSpeed,
                   double gravity, ms_t lifespan,
                   const ParticleProfile &profile)
    : Sprite(&ENTITY_TYPE, loc),
      _image(image),
      _drawRect(drawRect),
      _velocity(velocity),
      _altitude(startingAltitude),
      _fallSpeed(startingFallSpeed),
      _gravity(gravity),
      _lifespan(lifespan),
      _profile(profile) {}

void Particle::update(double delta) {
  _altitude -= _fallSpeed * delta;
  if (_altitude < 0 && !_profile.canBeUnderground()) {
    _altitude = 0;
    _velocity = {};
  }
  _fallSpeed += _gravity * delta;

  location(location() + _velocity * delta);

  ms_t timeElapsed = toInt(1000.0 * delta);
  if (timeElapsed > _lifespan)
    markForRemoval();
  else
    _lifespan -= timeElapsed;
}

void Particle::draw(const Client &client) const {
  // Draw only the top if partially underground
  auto drawRect = this->drawRect();
  auto srcRect = ScreenRect{0, 0, drawRect.w, drawRect.h};
  if (_altitude < 0) {
    auto cutoff = toInt(-_altitude);
    srcRect.h -= cutoff;
    drawRect.h -= cutoff;
  }

  _image.draw(drawRect + client.offset(), srcRect);
}

void Particle::setImageManually(const Texture &image) {
  _image = image;
  _drawRect.w = image.width();
  _drawRect.h = image.height();
  _drawRect.x = -_drawRect.w / 2;
  _drawRect.y = -_drawRect.h / 2;
}

ScreenRect Particle::drawRect() const {
  auto includingAltitude = location() - MapPoint{0, _altitude};
  return _drawRect + toScreenRect(includingAltitude);
}
