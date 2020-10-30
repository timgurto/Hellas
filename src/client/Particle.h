#ifndef PARTICLE_H
#define PARTICLE_H

#include "../NormalVariable.h"
#include "../Point.h"
#include "Sprite.h"
#include "SpriteType.h"

class ParticleProfile;
class Texture;

// An individual particle
class Particle : public Sprite {
  static const SpriteType ENTITY_TYPE;

  Texture _image;
  ScreenRect _drawRect;

  // x and y
  MapPoint _velocity;  // in px/s

  // z (height)
  double _altitude;
  double _fallSpeed;
  double _gravity;

  ms_t _timeRemaining;
  ms_t _timeSinceSpawn{0};

  const ParticleProfile &_profile;

 public:
  Particle(const MapPoint &loc, const Texture &image,
           const ScreenRect &drawRect, const MapPoint &velocity,
           double startingAltitude, double startingFallSpeed, double gravity,
           ms_t lifespan, const ParticleProfile &profile, Client &client);

  virtual ScreenRect drawRect() const override;
  virtual const Texture &image() const override { return _image; }
  void update(double delta) override;
  void draw() const override;
  void addToAltitude(double extra) { _altitude += extra; }
  void setImageManually(const Texture &image);
};

#endif
