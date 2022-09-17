#include "DamageOnUse.h"

#include "../util.h"
#include "Objects/Object.h"
#include "Server.h"
#include "ServerItem.h"

void DamageOnUse::onUseAsTool() {
  if (isBroken()) return;

  if (randDouble() < chanceToGetDamagedOnUseAsTool()) damageFromUse();
}

void DamageOnUse::onUseInCombat() {
  if (isBroken()) return;

  if (randDouble() < chanceToGetDamagedOnUseInCombat()) {
    damageFromUse();
  }
}

double Object::chanceToGetDamagedOnUseAsTool() const {
  if (objType().destroyIfUsedAsTool()) return 1.0;
  return DamageOnUse::chanceToGetDamagedOnUseAsTool();
}

void Object::damageFromUse() {
  if (objType().destroyIfUsedAsTool())
    kill();
  else
    reduceHealth(DAMAGE_ON_USE_AS_TOOL);
}

void Object::repair() { healBy(stats().maxHealth); }

void ServerItem::Instance::damageFromUse() {
  --_health;
  _reportingInfo.report();
}

void ServerItem::Instance::damageOnPlayerDeath() {
  const auto DAMAGE_ON_DEATH_RATE = .05;  // 5%.
  const auto rawDamage = DAMAGE_ON_DEATH_RATE * _type->maxHealth();
  if (rawDamage >= _health)
    _health = 0;
  else
    // De-facto minimum of 1 damage due to int -= double
    _health = static_cast<Hitpoints>(_health - rawDamage);

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
