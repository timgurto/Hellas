#include "ClientCombatant.h"

#include "Client.h"
#include "ClientCombatantType.h"
#include "Renderer.h"

extern Renderer renderer;

ClientCombatant::ClientCombatant(Client &client,
                                 const ClientCombatantType *type)
    : _cClient(client), _type(type) {
  if (!_type) return;
  _maxHealth = _type->maxHealth();
  _health = _maxHealth;
  _maxEnergy = _type->maxEnergy();
  _energy = _maxEnergy;
}

void ClientCombatant::update(double delta) { createBuffParticles(delta); }

void ClientCombatant::drawHealthBarIfAppropriate(const MapPoint &objectLocation,
                                                 px_t objHeight) const {
  if (!shouldDrawHealthBar()) return;

  static const px_t BAR_TOTAL_LENGTH = 10, BAR_HEIGHT = 2,
                    BAR_GAP =
                        4;  // Gap between the bar and the top of the sprite
  px_t barLength = toInt(1.0 * BAR_TOTAL_LENGTH * health() / maxHealth());
  const ScreenPoint &offset = _cClient.offset();
  px_t x = toInt(objectLocation.x - BAR_TOTAL_LENGTH / 2 + offset.x),
       y = toInt(objectLocation.y - objHeight - BAR_GAP - BAR_HEIGHT +
                 offset.y);

  renderer.setDrawColor(Color::UI_OUTLINE);
  renderer.drawRect({x - 1, y - 1, BAR_TOTAL_LENGTH + 2, BAR_HEIGHT + 2});
  renderer.setDrawColor(healthBarColor());
  renderer.fillRect({x, y, barLength, BAR_HEIGHT});
  renderer.setDrawColor(Color::UI_OUTLINE);
  renderer.fillRect(
      {x + barLength, y, BAR_TOTAL_LENGTH - barLength, BAR_HEIGHT});
}

bool ClientCombatant::shouldDrawHealthBar() const {
  if (!isAlive()) return false;
  bool isDamaged = health() < maxHealth();
  if (isDamaged) return true;
  if (canBeAttackedByPlayer()) return true;

  bool selected = _cClient.targetAsCombatant() == this;
  bool mousedOver = _cClient.currentMouseOverEntity() == entityPointer();
  if (selected || mousedOver) return true;

  return false;
}

void ClientCombatant::createDamageParticles() const {
  _cClient.addParticles(_type->damageParticles(), combatantLocation());
}

void ClientCombatant::createBuffParticles(double delta) const {
  for (auto *buff : _buffs)
    if (!buff->particles().empty())
      _cClient.addParticles(buff->particles(), combatantLocation(), delta);
  for (auto *debuff : _debuffs)
    if (!debuff->particles().empty())
      _cClient.addParticles(debuff->particles(), combatantLocation(), delta);
}

void ClientCombatant::addBuffOrDebuff(const ClientBuffType::ID &buff,
                                      bool isBuff) {
  auto it = _cClient.gameData.buffTypes.find(buff);
  if (it == _cClient.gameData.buffTypes.end()) return;

  const auto &buffType = it->second;

  if (isBuff)
    _buffs.insert(&buffType);
  else
    _debuffs.insert(&buffType);
}

void ClientCombatant::removeBuffOrDebuff(const ClientBuffType::ID &buff,
                                         bool isBuff) {
  auto it = _cClient.gameData.buffTypes.find(buff);
  if (it == _cClient.gameData.buffTypes.end()) return;

  if (isBuff)
    _buffs.erase(&it->second);
  else
    _debuffs.erase(&it->second);
}

void ClientCombatant::playSoundWhenHit() const {
  // Never true at this point for Avatar.  That case is handled specially, with
  // SV_A_PLAYER_DIED.
  if (isDead())
    playDeathSound();
  else
    playDefendSound();
}

void ClientCombatant::drawBuffEffects(const MapPoint &location,
                                      const ScreenPoint &clientOffset) const {
  const auto delta = _cClient._timeElapsed / 1000.0;
  for (auto *buffType : buffs()) {
    if (!buffType->particles().empty())
      _cClient.addParticles(buffType->particles(), location, delta);

    if (buffType->hasEffect())
      buffType->effectImage().draw(toScreenPoint(location) + clientOffset +
                                   buffType->effectOffset());
  }
}

bool ClientCombatant::doesAnyBuffHideMe() const {
  for (auto *buffType : buffs()) {
    if (buffType->isTargetInvisible()) return true;
  }
  return false;
}
