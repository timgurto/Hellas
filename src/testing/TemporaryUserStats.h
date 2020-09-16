#pragma once

#include "../server/User.h"

// This class acts as a size-one stack.  Use it to push altered User base stats.
// The original stats will be restored automatically when the object is
// destroyed.
class TemporaryUserStats {
 public:
  TemporaryUserStats();
  ~TemporaryUserStats();

  TemporaryUserStats &hps(Regen v);
  TemporaryUserStats &hit(BasisPoints v);
  TemporaryUserStats &crit(BasisPoints v);
  TemporaryUserStats &critResist(BasisPoints v);
  TemporaryUserStats &dodge(BasisPoints v);
  TemporaryUserStats &block(BasisPoints v);
  TemporaryUserStats &armour(ArmourClass v);
  TemporaryUserStats &weaponDamage(Hitpoints v);
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
