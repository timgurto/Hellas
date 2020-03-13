#include "DamageOnUse.h"

#include "../util.h"

void DamageOnUse::onUse() {
  if (isBroken()) return;

  if (randDouble() > chanceToGetDamagedOnUse()) return;

  damageFromUse();
}
