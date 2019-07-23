#include "DamageOnUse.h"

#include "../util.h"

void DamageOnUse::onUse() {
  if (isBroken()) return;

  // This line temporarily disables all damage
  return;

  const auto DAMAGE_CHANCE = 0.05;
  if (randDouble() > DAMAGE_CHANCE) return;

  damageFromUse();
}
