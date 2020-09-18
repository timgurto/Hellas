#pragma once

#include "../server/User.h"

// This class acts as a size-one stack.  Use it to push altered User base stats.
// The original stats will be restored automatically when the object is
// destroyed.
class TemporaryUserStats {
 public:
  TemporaryUserStats();
  ~TemporaryUserStats();

  TemporaryUserStats &maxHealth(Hitpoints v);
  TemporaryUserStats &hps(Regen v);
  TemporaryUserStats &hit(BasisPoints v);
  TemporaryUserStats &crit(BasisPoints v);
  TemporaryUserStats &critResist(BasisPoints v);
  TemporaryUserStats &dodge(BasisPoints v);
  TemporaryUserStats &block(BasisPoints v);
  TemporaryUserStats &blockValue(Hitpoints v);
  TemporaryUserStats &armour(ArmourClass v);
  TemporaryUserStats &weaponDamage(Hitpoints v);
  TemporaryUserStats &weaponSchool(SpellSchool v);
  TemporaryUserStats &physicalDamage(BasisPoints v);
  TemporaryUserStats &magicDamage(BasisPoints v);
  TemporaryUserStats &healing(BasisPoints v);
  TemporaryUserStats &gatherBonus(Percentage v);
  TemporaryUserStats &followerLimit(int v);

 private:
  void apply();
  void storeCurrentSettings();
  void restoreOriginalSettings();
  Stats _new;
  Stats _old;
};

#define CHANGE_BASE_USER_STATS           \
  TemporaryUserStats temporaryUserStats; \
  temporaryUserStats
