// (C) 2016 Tim Gurto

#include "Particle.h"
#include "../util.h"

const EntityType Particle::ENTITY_TYPE; // The default constructor will suffice.

const double Particle::GRAVITY = 100.0;

Particle::Particle(const Point &loc, const Texture &image, const Rect &drawRect,
                   const Point &velocity, double startingAltitude, double startingFallSpeed):
Entity(&ENTITY_TYPE, loc),
_image(image),
_drawRect(drawRect),
_velocity(velocity),
_altitude(startingAltitude),
_fallSpeed(startingFallSpeed)
{}

void Particle::update(double delta){
    _altitude -= _fallSpeed * delta;
    if (_altitude <= 0){
        markForRemoval();
        return;
    }
    _fallSpeed += GRAVITY * delta;

    location(location() + _velocity * delta);
}

Rect Particle::drawRect() const{
    Rect rect = _drawRect + location();
    rect.y = toInt(rect.y - _altitude);
    return rect;
}