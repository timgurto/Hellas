#include "Client.h"
#include "Projectile.h"
#include "SoundProfile.h"

void Projectile::update(double delta) {
    auto distanceToMove = speed() * delta;
    auto distanceFromTarget = distance(location(), _end);
    auto reachedTarget = distanceToMove >= distanceFromTarget;

    if (reachedTarget) {

        if (!particlesAtEnd().empty())
            Client::instance().addParticles(particlesAtEnd(), _end);

        auto sounds = projectileType().sounds();
        if (sounds)
            sounds->playOnce("impact"s);

        markForRemoval();
        return;
    }

    auto newLocation = interpolate(location(), _end, distanceToMove);
    location(newLocation);
}

void Projectile::Type::sounds(const std::string & profile) {
    _sounds = Client::instance().findSoundProfile(profile);
}
