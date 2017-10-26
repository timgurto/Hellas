#include "Projectile.h"

void Projectile::update(double delta) {
    auto distanceToMove = speed() * delta;
    auto distanceFromTarget = distance(location(), _end);
    if (distanceToMove >= distanceFromTarget) {
        markForRemoval();
        return;
    }

    auto newLocation = interpolate(location(), _end, distanceToMove);
    location(newLocation);
}
