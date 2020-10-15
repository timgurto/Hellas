#ifndef CLIENT_COMBATANT_H
#define CLIENT_COMBATANT_H

#include <cassert>

#include "../types.h"
#include "ClientBuff.h"
#include "ClientCombatantType.h"
#include "Sprite.h"

class List;

class ClientCombatant {
 public:
  ClientCombatant::ClientCombatant(Client &client,
                                   const ClientCombatantType *type);

  void update(double delta);  // Non-virtual.

  const Hitpoints &health() const { return _health; }
  void health(Hitpoints n) { _health = n; }
  const Energy &energy() const { return _energy; }
  void energy(Energy n) { _energy = n; }
  bool isAlive() const { return _health > 0; }
  bool isDead() const { return _health == 0; }
  const Hitpoints &maxHealth() const { return _maxHealth; }
  const Energy &maxEnergy() const { return _maxEnergy; }
  void maxHealth(Hitpoints newMax) { _maxHealth = newMax; }
  void maxEnergy(Energy newMax) { _maxEnergy = newMax; }
  void drawHealthBarIfAppropriate(const MapPoint &objectLocation,
                                  px_t objHeight) const;
  const Level &level() const { return _level; }
  void level(Level l) { _level = l; }
  ClientCombatantType::Rank rank() const { return _type->rank(); }

  virtual void sendTargetMessage() const = 0;
  virtual void sendSelectMessage() const = 0;
  virtual bool canBeAttackedByPlayer() const { return isAlive(); }
  virtual const Sprite *entityPointer() const = 0;
  virtual const MapPoint &combatantLocation() const = 0;
  virtual bool shouldDrawHealthBar() const;
  virtual const Color &healthBarColor() const = 0;
  void drawBuffEffects(const MapPoint &location,
                       const ScreenPoint &clientOffset) const;

  virtual void addMenuButtons(List &menu) {}

  void createDamageParticles() const;
  void createBuffParticles(double delta) const;

  void addBuffOrDebuff(const ClientBuffType::ID &buff, bool isBuff);
  void removeBuffOrDebuff(const ClientBuffType::ID &buff, bool isBuff);

  size_t numBuffs() const { return _buffs.size() + _debuffs.size(); }
  using Buffs = std::set<const ClientBuffType *>;
  const Buffs &buffs() const { return _buffs; }
  const Buffs &debuffs() const { return _debuffs; }

  virtual void playAttackSound() const = 0;
  void playSoundWhenHit() const;
  virtual void playDefendSound() const = 0;
  virtual void playDeathSound() const = 0;

 private:
  Client &_cClient;

  const ClientCombatantType *_type;
  Hitpoints _maxHealth, _health;
  Energy _maxEnergy, _energy;
  Buffs _buffs{}, _debuffs{};
  Level _level{0};
};

#endif
