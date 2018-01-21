#ifndef PARTICLE_H
#define PARTICLE_H

#include "Sprite.h"
#include "SpriteType.h"
#include "../NormalVariable.h"
#include "../Point.h"

class ParticleProfile;
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

    const ParticleProfile &_profile;

public:
    Particle(const MapPoint &loc, const Texture &image, const ScreenRect &drawRect, const MapPoint &velocity,
             double startingAltitude, double startingFallSpeed, double gravity, ms_t lifespan,
             const ParticleProfile &profile);

    ScreenRect drawRect() const override;
    const Texture &image() const override { return _image; }
    void update(double delta) override;
    void draw(const Client &client) const override;
    void addToAltitude(double extra) { _altitude += extra; }
};

#endif
