#ifndef CLIENT_COMBATANT_H
#define CLIENT_COMBATANT_H

#include <cassert>

#include "ClientCombatantType.h"
#include "Sprite.h"
#include "../types.h"

class List;

class ClientCombatant{
public:
    ClientCombatant::ClientCombatant(const ClientCombatantType *type);

    const health_t &health() const { return _health; }
    void health(health_t n) { _health = n; }
    bool isAlive() const { return _health > 0; }
    bool isDead() const { return _health == 0; }
    const health_t &maxHealth() const { return _maxHealth; }
    void maxHealth(health_t newMax) { _maxHealth = newMax; }
    void drawHealthBarIfAppropriate(const Point &objectLocation, px_t objHeight) const;

    virtual void sendTargetMessage() const = 0;
    virtual bool canBeAttackedByPlayer() const { return isAlive(); }
    virtual const Sprite *entityPointer() const = 0;
    virtual const Point &combatantLocation() const = 0;
    virtual bool shouldDrawHealthBar() const;
    virtual const Color &healthBarColor() const = 0;

    virtual void addMenuButtons(List &menu) const{}

    void createDamageParticles() const;

private:
    const ClientCombatantType *_type;
    health_t _maxHealth;
    health_t _health;
};

#endif
