#include "Projectile.h"
#include "Client.h"
#include "SoundProfile.h"

void Projectile::update(double delta) {
  auto distanceToMove = speed() * delta;
  auto distanceFromTarget = distance(location(), _end);
  auto reachedTarget = distanceToMove >= distanceFromTarget;

  if (reachedTarget) {
    auto shouldShowImpact = !_willMiss;
    if (shouldShowImpact) {
      if (!particlesAtEnd().empty())
        Client::instance().addParticles(particlesAtEnd(), _end);

      auto sounds = projectileType().sounds();
      if (sounds) sounds->playOnce("impact"s);
    }

    markForRemoval();
    for (auto segment : _tail) segment->markForRemoval();
    return;
  }

  const auto &tailParticles = projectileType()._tailParticles;

  auto newLocation = interpolate(location(), _end, distanceToMove);
  auto locationDelta = newLocation - location();
  location(newLocation);
  for (auto segment : _tail) {
    // Update location
    auto oldSegmentLocation = segment->location();
    segment->location(oldSegmentLocation + locationDelta);
    segment->destination(segment->location());

    // Add particles
    if (tailParticles != ""s) {
      Client::instance().addParticles(tailParticles, segment->location());
    }
  }
}

void Projectile::Type::tail(const std::string &imageFile,
                            const ScreenRect &drawRect, int length,
                            int separation, const std::string &particles) {
  _tailType = {drawRect, "Images/Projectiles/"s + imageFile + ".png"s};
  _tailLength = length;
  _tailSeparation = separation;
  _tailParticles = particles;
}

void Projectile::Type::sounds(const std::string &profile) {
  _sounds = Client::instance().findSoundProfile(profile);
}

void Projectile::Type::instantiate(const MapPoint &start, const MapPoint &end,
                                   bool willMiss) const {
  auto &client = Client::instance();
  auto projectile = new Projectile(*this, start, end);
  if (willMiss) projectile->willMiss();
  client.addEntity(projectile);

  // Add tail
  auto dist = distance(start, end);
  for (auto i = 0; i != _tailLength; ++i) {
    dist += _tailSeparation;
    auto position = extrapolate(end, start, dist);
    auto tailSegment = new Sprite(&_tailType, position);
    client.addEntity(tailSegment);
    projectile->_tail.push_back(tailSegment);
  }
}
