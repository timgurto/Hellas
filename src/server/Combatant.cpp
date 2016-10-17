// (C) 2016 Tim Gurto

#include <cassert>

#include "Combatant.h"
#include "Server.h"

Combatant::Combatant(health_t health):
_health(health)
{}

health_t Combatant::reduceHealth(health_t damage){
    assert (_health > 0);

    if (damage >= _health)
        _health = 0;
    else
        _health -= damage;

    return _health;
}
