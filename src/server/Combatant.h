#ifndef COMBATANT_H
#define COMBATANT_H

#include "../Rect.h"
#include "../types.h"

// Abstract class describing something that can participate in combat with another Combatant.
class Combatant{
    health_t _health;
    ms_t _attackTimer;
    Combatant *_target;

public:
    Combatant(health_t health = 0);
    virtual ~Combatant(){}

    virtual health_t maxHealth() const = 0;
    virtual health_t attack() const = 0;
    virtual ms_t attackTime() const = 0;
    virtual Rect hitbox() const = 0;
    virtual const Point &getLocation() const = 0;
    Combatant *target() const { return _target; }
    void target(Combatant *p) { _target = p; }

    health_t health() const { return _health; }
    void health(health_t health) { _health = health; }

    health_t reduceHealth(health_t damage); // Returns remaining health.

    virtual void onHealthChange() {}; // Probably alerting relevant users.
    virtual void onDeath() {}; // Anything that needs to happen upon death.

    void updateCombat(ms_t timeElapsed);
};

#endif
