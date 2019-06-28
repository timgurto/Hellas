#pragma once

#include "../types.h"

class DamageOnUse {
 public:
  virtual bool isBroken() const = 0;

  // Return value: whether item health changed as a result. Whenever true, the
  // owner should be udpated.
  bool onUse();

  virtual void damageFromUse() = 0;
};
