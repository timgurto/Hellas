#pragma once

#include "../types.h"

class DamageOnUse {
 public:
  virtual bool isBroken() const = 0;

  // Return value: whether item health changed as a result. Whenever true, the
  // owner should be udpated.
  void onUseAsTool();
  void onUseInCombat();

  virtual double chanceToGetDamagedOnUseAsTool() const { return 0.25; }
  virtual double chanceToGetDamagedOnUseInCombat() const { return 0.01; }
  virtual void damageFromUse() = 0;
  virtual void damageOnPlayerDeath() {}

  virtual void repair() = 0;

  virtual double toolSpeed(const std::string &tag) const { return 1.0; }
};
