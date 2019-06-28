#include "DamageOnUse.h"

#include "../util.h"

bool DamageOnUse::onUse() {
  if (isBroken()) return false;

  const auto DAMAGE_CHANCE = 0.05;
  if (randDouble() > DAMAGE_CHANCE) return false;

  damageFromUse();
  return true;
}
