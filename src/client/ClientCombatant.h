#ifndef CLIENT_COMBATANT_H
#define CLIENT_COMBATANT_H

#include "ClientCombatantType.h"
#include "Entity.h"
#include "../types.h"

class ClientCombatant{
public:
    ClientCombatant::ClientCombatant(const ClientCombatantType *type);

    health_t health() const { return _health; }
    void health(health_t n) { _health = n; }
    bool isAlive() const { return _health > 0; }
    bool isDead() const { return _health == 0; }
    health_t maxHealth() const { return _type->maxHealth(); }
    
    virtual void sendTargetMessage() const {}

private:
    health_t _health;
    const ClientCombatantType *_type;
};

#endif
