#include <cassert>

#include "Combatant.h"
#include "Server.h"

Combatant::Combatant(const ObjectType *type, const Point &loc, health_t health):
Object(type, loc),
_health(health),
_attackTimer(0),
_target(nullptr)
{}

void Combatant::reduceHealth(health_t damage){
    if (damage >= _health) {
        _health = 0;
        onDeath();
    } else if (damage != 0) {
        _health -= damage;
    }
}

void Combatant::update(ms_t timeElapsed){
    if (_attackTimer > timeElapsed)
        _attackTimer -= timeElapsed;
    else
        _attackTimer = 0;

    assert(target()->health() > 0);

    if (_attackTimer > 0)
        return;

    // Check if within range
    if (distance(collisionRect(), target()->collisionRect()) <= Server::ACTION_DISTANCE){

        // Reduce target health (to minimum 0)
        target()->reduceHealth(attack());
        target()->onHealthChange();

        // Reset timer
        _attackTimer = attackTime();
    }
}
