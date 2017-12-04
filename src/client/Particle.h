#ifndef PARTICLE_H
#define PARTICLE_H

#include "Sprite.h"
#include "SpriteType.h"
#include "../NormalVariable.h"
#include "../Point.h"

class Texture;

// An individual particle
class Particle : public Sprite{
    static const SpriteType ENTITY_TYPE;

    const Texture &_image;
    const ScreenRect &_drawRect;

    // x and y
    MapPoint _velocity; // in px/s

    // z (height)
    double _altitude;
    double _fallSpeed;
    double _gravity;

    ms_t _lifespan;

public:
    Particle(const MapPoint &loc, const Texture &image, const ScreenRect &drawRect, const MapPoint &velocity,
             double startingAltitude, double startingFallSpeed, double gravity, ms_t lifespan);

    virtual ScreenRect drawRect() const override;
    virtual const Texture &image() const override { return _image; }
    virtual void update(double delta) override;
};

#endif
