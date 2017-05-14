#ifndef ENTITY_H
#define ENTITY_H

#include "objects/Object.h"
#include "../Rect.h"
#include "../types.h"

// Abstract class describing something that can participate in combat with another Entity.
class Entity : public Object{
    health_t _health;
    ms_t _attackTimer;
    Entity *_target;

public:
    Entity(const ObjectType *type, const Point &loc, health_t health = 0);

    // For lookup dummies
    Entity(){}
    Entity(const Point &loc): Object(loc){}

    virtual ~Entity(){}

    virtual health_t maxHealth() const = 0;
    virtual health_t attack() const = 0;
    virtual ms_t attackTime() const = 0;
    Entity *target() const { return _target; }
    void target(Entity *p) { _target = p; }

    health_t health() const { return _health; }
    void health(health_t health) { _health = health; }

    void reduceHealth(int damage);

    virtual void onHealthChange() {}; // Probably alerting relevant users.
    virtual void onDeath() {}; // Anything that needs to happen upon death.

    virtual void update(ms_t timeElapsed) override;
};

#endif
