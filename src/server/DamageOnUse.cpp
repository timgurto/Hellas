#include "DamageOnUse.h"

#include "../util.h"

void DamageOnUse::onUse() {
  if (isBroken()) return;

  const auto DAMAGE_CHANCE = 0.1;
  if (randDouble() > DAMAGE_CHANCE) return;

  damageFromUse();
}
