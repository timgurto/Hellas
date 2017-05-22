#ifndef CLIENT_COMBATANT_TYPE_H
#define CLIENT_COMBATANT_TYPE_H

#include "../types.h"

class ClientCombatantType{
public:
    ClientCombatantType();
    ClientCombatantType(health_t maxHealth);

    const health_t &maxHealth() const { return _maxHealth; }
    void maxHealth(health_t n) { _maxHealth = n; }

private:
    health_t _maxHealth;
};

#endif
