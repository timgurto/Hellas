// (C) 2016 Tim Gurto

#ifndef PARTICLE_H
#define PARTICLE_H

#include "Entity.h"
#include "EntityType.h"
#include "../NormalVariable.h"
#include "../Point.h"

class Texture;

// An individual particle
class Particle : public Entity{
    static Texture *images[3];
    static const Rect DRAW_RECT;
    static const EntityType ENTITY_TYPE;
    static const NormalVariable
        startingDistanceGenerator,
        startingAltitudeGenerator,
        velocityGenerator,
        startingFallSpeedGenerator;

    size_t _imageType; // Random

    // x and y
    Point _velocity; // in px/s

    // z (height)
    double _altitude;
    double _fallSpeed;

    static const double
        GRAVITY, // px/s/s
        DISTANCE_MEAN, // Distance from center
        DISTANCE_SD,
        ALTITUDE_MEAN, // Height from base
        ALTITUDE_SD,
        VELOCITY_MEAN, // In random lateral (x,y) direction
        VELOCITY_SD,
        FALL_SPEED_MEAN,
        FALL_SPEED_SD;

public:
    Particle(const Point &loc);

    virtual Rect drawRect() const override;
    virtual const Texture &image() const override;
    virtual void update(double delta) override;
    
    static void init();
    static void quit();
};

#endif
