#include "Particle.h"
#include "../util.h"

const SpriteType Particle::ENTITY_TYPE(SpriteType::DECORATION);

Particle::Particle(const MapPoint &loc, const Texture &image, const ScreenRect &drawRect,
                   const MapPoint &velocity, double startingAltitude, double startingFallSpeed,
                   double gravity, ms_t lifespan):
Sprite(&ENTITY_TYPE, loc),
_image(image),
_drawRect(drawRect),
_velocity(velocity),
_altitude(startingAltitude),
_fallSpeed(startingFallSpeed),
_gravity(gravity),
_lifespan(lifespan)
{}

void Particle::update(double delta){
    _altitude -= _fallSpeed * delta;
    if (_altitude < 0){
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

ScreenRect Particle::drawRect() const{
    auto includingAltitude = location() - MapPoint{ 0, _altitude };
    return _drawRect + toScreenRect(includingAltitude);
}