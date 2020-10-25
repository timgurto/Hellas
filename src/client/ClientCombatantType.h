#ifndef CLIENT_COMBATANT_TYPE_H
#define CLIENT_COMBATANT_TYPE_H

#include "../combatTypes.h"

class ParticleProfile;

class ClientCombatantType {
 public:
  ClientCombatantType() {}
  ClientCombatantType(Hitpoints maxHealth);

  const Hitpoints &maxHealth() const { return _maxHealth; }
  const Energy &maxEnergy() const { return _maxEnergy; }
  void maxHealth(Hitpoints n) { _maxHealth = n; }
  const ParticleProfile *damageParticles() const { return _damageParticles; }
  void damageParticles(const ParticleProfile *profile) {
    _damageParticles = profile;
  }

  enum Rank { COMMON, ELITE, BOSS };
  void makeElite() { _rank = ELITE; }
  void makeBoss() { _rank = BOSS; }
  Rank rank() const { return _rank; }

 private:
  Hitpoints _maxHealth = 0;
  Energy _maxEnergy = 0;
  const ParticleProfile *_damageParticles = nullptr;
  Rank _rank{COMMON};
};

#endif
