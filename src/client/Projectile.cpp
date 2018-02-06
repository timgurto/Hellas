#include "Client.h"
#include "Projectile.h"

void Projectile::update(double delta) {
    auto distanceToMove = speed() * delta;
    auto distanceFromTarget = distance(location(), _end);
    auto reachedTarget = distanceToMove >= distanceFromTarget;

    if (reachedTarget) {

        if (!particlesAtEnd().empty())
            Client::instance().addParticles(particlesAtEnd(), _end);

        markForRemoval();
        return;
    }

    auto newLocation = interpolate(location(), _end, distanceToMove);
    location(newLocation);
}
