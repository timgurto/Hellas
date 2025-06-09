#pragma once

#include "../types.h"

class DamageOnUse {
 public:
  virtual bool isBroken() const = 0;

  // Return value: whether item health changed as a result. Whenever true, the
  // owner should be udpated.
  void onUseAsTool();
  void onUseInCombat();

  // Don't change this lightly.  It has been factored into balancing the total
  // material costs of gathering/crafting.
  virtual double chanceToGetDamagedOnUseAsTool() const { return 0.25; }

  virtual double chanceToGetDamagedOnUseInCombat() const { return 0.01; }
  virtual void damageFromUse() = 0;
  virtual void damageOnPlayerDeath() {}

  virtual void repair() = 0;

  virtual double toolSpeed(const std::string &) const { return 1.0; }
};
