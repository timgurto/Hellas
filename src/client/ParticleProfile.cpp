#include "ParticleProfile.h"

#include <cassert>

#include "Particle.h"

const double ParticleProfile::DEFAULT_GRAVITY = 100.0;
const ms_t ParticleProfile::DEFAULT_LIFESPAN_MEAN = 60000;
const ms_t ParticleProfile::DEFAULT_LIFESPAN_SD = 10000;

ParticleProfile::ParticleProfile(const std::string &id) : _id(id) {}

ParticleProfile::~ParticleProfile() {
  for (const SpriteType *variety : _varieties) delete variety;
}

void ParticleProfile::addVariety(const std::string &imageFile, size_t count,
                                 const ScreenRect &drawRect) {
  SpriteType *particleType =
      new SpriteType(drawRect, "Images/Particles/" + imageFile);
  if (_alpha != 0xff) particleType->setAlpha(0x7f);
  _varieties.push_back(particleType);  // _varieties owns the pointers.
  for (size_t i = 0; i != count; ++i) _pool.push_back(particleType);
}

void ParticleProfile::addVariety(const std::string &imageFile, size_t count) {
  SpriteType *particleType =
      new SpriteType({}, "Images/Particles/" + imageFile);
  auto editedRect = particleType->drawRect();
  editedRect.x = -particleType->width() / 2;
  editedRect.y = -particleType->height() / 2;
  particleType->drawRect(editedRect);
  if (_alpha != 0xff) particleType->setAlpha(_alpha);
  _varieties.push_back(particleType);  // _varieties owns the pointers.
  for (size_t i = 0; i != count; ++i) _pool.push_back(particleType);
}

Particle *ParticleProfile::instantiate(const MapPoint &location,
                                       Client &client) const {
  // Choose random variety
  assert(!_pool.empty());
  size_t index = rand() % _pool.size();
  const SpriteType &variety = *_pool[index];

  // Choose random position, then set location
  double angle = 2 * PI * randDouble();
  double distance = _distance.generate();
  MapPoint locationOffset(cos(angle) * distance, sin(angle) * distance);
  MapPoint startingLoc = location + locationOffset;

  double velocity = _velocity.generate();
  auto startingVelocity = MapPoint{};
  if (convergesToCentre()) {
    velocity = abs(velocity);
    const auto directionVector = -locationOffset / distance;
    startingVelocity = directionVector * velocity;
  } else {
    // Initialize velocity from base direction
    startingVelocity = _direction;

    // Modify by random direction
    angle = 2 * PI * randDouble();
    startingVelocity += {cos(angle) * velocity, sin(angle) * velocity};
  }

  if (_noZDimension) {
    startingLoc.y = location.y;
    startingVelocity.y = 0;
  }

  const auto lifespan = max(0, toInt(_lifespan.generate()));

  return new Particle(startingLoc, variety.image(), variety.drawRect(),
                      startingVelocity, _altitude.generate(),
                      _fallSpeed.generate(), _gravity, lifespan, *this, client);
}

size_t ParticleProfile::numParticlesContinuous(double delta) const {
  size_t total = 0;
  double particlesToAdd = _particlesPerSecond * delta;
  if (particlesToAdd >= 1) {  // Whole part
    total = static_cast<size_t>(particlesToAdd);
    particlesToAdd -= total;
  }
  if (randDouble() <= particlesToAdd) ++total;
  return total;
}

size_t ParticleProfile::numParticlesDiscrete() const {
  return max(0, toInt(_particlesPerHit.generate()));
}
