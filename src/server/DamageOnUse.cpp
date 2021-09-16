#include "DamageOnUse.h"

#include "../util.h"
#include "Objects/Object.h"
#include "ServerItem.h"

void DamageOnUse::onUseAsTool() {
  if (isBroken()) return;

  if (randDouble() < chanceToGetDamagedOnUseAsTool()) damageFromUse();
}

void DamageOnUse::onUseInCombat() {
  if (isBroken()) return;

  if (randDouble() < chanceToGetDamagedOnUseInCombat()) damageFromUse();
}

double Object::chanceToGetDamagedOnUseAsTool() const {
  if (objType().destroyIfUsedAsTool()) return 1.0;
  return DamageOnUse::chanceToGetDamagedOnUseAsTool();
}

void Object::damageFromUse() { reduceHealth(100); };

void Object::repair() { healBy(stats().maxHealth); }

void ServerItem::Instance::damageFromUse() {
  --_health;
  _reportingInfo.report();
}

void ServerItem::Instance::damageOnPlayerDeath() {
  const auto DAMAGE_ON_DEATH = Hitpoints{5};
  if (DAMAGE_ON_DEATH >= _health)
    _health = 0;
  else
    _health -= DAMAGE_ON_DEATH;

  _reportingInfo.report();
}

void ServerItem::Instance::repair() {
  _health = _type->maxHealth();
  _reportingInfo.report();
}

double ServerItem::Instance::toolSpeed(const std::string &tag) const {
  const auto &tags = _type->tags();
  auto it = tags.find(tag);
  if (it == tags.end()) return 1.0;

  return it->second;
}
