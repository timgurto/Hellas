#include "Particle.h"
#include "../util.h"

const EntityType Particle::ENTITY_TYPE; // The default constructor will suffice.

Particle::Particle(const Point &loc, const Texture &image, const Rect &drawRect,
                   const Point &velocity, double startingAltitude, double startingFallSpeed,
                   double gravity, ms_t lifespan):
Entity(&ENTITY_TYPE, loc),
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
        markForRemoval();
        return;
    }
    _fallSpeed += _gravity * delta;

    location(location() + _velocity * delta);

    ms_t timeElapsed = toInt(1000.0 * delta);
    if (timeElapsed > _lifespan)
        markForRemoval();
    else
        _lifespan -= timeElapsed;

}

Rect Particle::drawRect() const{
    Rect rect = _drawRect + location();
    rect.y = toInt(rect.y - _altitude);
    return rect;
}