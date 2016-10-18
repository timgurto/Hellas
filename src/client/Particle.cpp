// (C) 2016 Tim Gurto

#include "Particle.h"
#include "../util.h"

Texture *Particle::images[3];

const EntityType Particle::ENTITY_TYPE; // Default constructor will suffice.
const Rect Particle::DRAW_RECT(-2, -2, 4, 4);

const double Particle::GRAVITY = 100;

const double Particle::DISTANCE_MEAN = 5;
const double Particle::DISTANCE_SD = 0;
const double Particle::ALTITUDE_MEAN = 20;
const double Particle::ALTITUDE_SD = 4;
const double Particle::VELOCITY_MEAN = 15;
const double Particle::VELOCITY_SD = 3;
const double Particle::FALL_SPEED_MEAN = 0;
const double Particle::FALL_SPEED_SD = 3;

const NormalVariable Particle::startingDistanceGenerator(DISTANCE_MEAN, DISTANCE_SD);
const NormalVariable Particle::startingAltitudeGenerator(ALTITUDE_MEAN, ALTITUDE_SD);
const NormalVariable Particle::velocityGenerator(VELOCITY_MEAN, VELOCITY_SD);
const NormalVariable Particle::startingFallSpeedGenerator(FALL_SPEED_MEAN, FALL_SPEED_SD);

Particle::Particle(const Point &loc):
Entity(&ENTITY_TYPE, loc),
_imageType(rand() % 3),
_altitude(startingAltitudeGenerator.generate()),
_fallSpeed(startingFallSpeedGenerator.generate())
{
    // Choose random direction, then set location
    double angle = 2 * PI * randDouble();
    double distance = startingDistanceGenerator.generate();
    Point locationOffset(cos(angle) * distance, sin(angle) * distance);
    location(location() + locationOffset);

    // Choose random direction, then set velocity
    angle = 2 * PI * randDouble();
    double velocity = velocityGenerator.generate();
    _velocity.x = cos(angle) * velocity;
    _velocity.y = sin(angle) * velocity;
}

void Particle::init(){
    images[0] = new Texture("Images/Particles/tree00.png", Color::MAGENTA);
    images[1] = new Texture("Images/Particles/tree01.png", Color::MAGENTA);
    images[2] = new Texture("Images/Particles/tree02.png", Color::MAGENTA);
}

void Particle::quit(){
    delete images[0];
    delete images[1];
    delete images[2];
}

Rect Particle::drawRect() const{
    Rect rect = DRAW_RECT + location();
    rect.y = toInt(rect.y - _altitude);
    return rect;
}

const Texture &Particle::image() const{
    return *images[_imageType];
}

void Particle::update(double delta){
    _altitude -= _fallSpeed * delta;
    if (_altitude <= 0){
        markForRemoval();
        return;
    }
    _fallSpeed += GRAVITY * delta;

    location(location() + _velocity * delta);
}
