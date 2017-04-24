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
    health_t maxHealth() const { return _type->maxHealth(); }

    const Entity *asEntity() const;

private:
    health_t _health;
    const ClientCombatantType *_type;
};

#endif
