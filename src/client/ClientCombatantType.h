#ifndef CLIENT_COMBATANT_TYPE_H
#define CLIENT_COMBATANT_TYPE_H

#include "../types.h"

class ParticleProfile;

class ClientCombatantType{
public:
    ClientCombatantType();
    ClientCombatantType(Hitpoints maxHealth);

    const Hitpoints &maxHealth() const { return _maxHealth; }
    void maxHealth(Hitpoints n) { _maxHealth = n; }
    const ParticleProfile *damageParticles() const { return _damageParticles; }
    void damageParticles(const ParticleProfile *profile) { _damageParticles = profile; }

private:
    Hitpoints _maxHealth;
    const ParticleProfile *_damageParticles;
};

#endif
