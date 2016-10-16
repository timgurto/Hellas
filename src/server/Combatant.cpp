// (C) 2016 Tim Gurto

#include "Combatant.h"

Combatant::Combatant(health_t health):
_health(health)
{}

health_t Combatant::reduceHealth(health_t damage){
    if (damage >= _health)
        _health = 0;
    else
        _health -= damage;
    return _health;
}
