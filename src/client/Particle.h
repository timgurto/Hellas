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
    static const EntityType ENTITY_TYPE;

    const Texture &_image;
    const Rect &_drawRect;

    // x and y
    Point _velocity; // in px/s

    // z (height)
    double _altitude;
    double _fallSpeed;

    static const double GRAVITY; // px/s/s

public:
    Particle(const Point &loc, const Texture &image, const Rect &drawRect, const Point &velocity,
             double startingAltitude, double startingFallSpeed);

    virtual Rect drawRect() const override;
    virtual const Texture &image() const override { return _image; }
    virtual void update(double delta) override;

    static size_t numParticlesToAdd(double delta);
};

#endif
