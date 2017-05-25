#ifndef CLIENT_COMBATANT_TYPE_H
#define CLIENT_COMBATANT_TYPE_H

#include "../types.h"

class ParticleProfile;

class ClientCombatantType{
public:
    ClientCombatantType();
    ClientCombatantType(health_t maxHealth);

    const health_t &maxHealth() const { return _maxHealth; }
    void maxHealth(health_t n) { _maxHealth = n; }
    const ParticleProfile *damageParticles() const { return _damageParticles; }
    void damageParticles(const ParticleProfile *profile) { _damageParticles = profile; }

private:
    health_t _maxHealth;
    const ParticleProfile *_damageParticles;
};

#endif
