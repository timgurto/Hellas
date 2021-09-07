#ifndef ENTITY_H
#define ENTITY_H

#include <memory>

#include "../Message.h"
#include "../Point.h"
#include "../Rect.h"
#include "../Serial.h"
#include "../SpellSchool.h"
#include "../types.h"
#include "Buff.h"
#include "EntityType.h"
#include "Gatherable.h"
#include "Loot.h"
#include "Permissions.h"
#include "ServerItem.h"
#include "Tagger.h"
#include "ThreatTable.h"

class Spawner;
class XmlWriter;
class NPC;
class User;

// Abstract class describing location, movement and combat functions of
// something in the game world
class Entity {
 public:
  Entity(const EntityType *type, const MapPoint &loc);
  Entity(Serial serial);        // TODO make private
  Entity(const MapPoint &loc);  // TODO make private
  virtual ~Entity();

  const EntityType *type() const { return _type; }

  virtual char classTag() const = 0;

  struct compareSerial {
    bool operator()(const Entity *a, const Entity *b) const;
  };
  struct compareXThenSerial {
    bool operator()(const Entity *a, const Entity *b) const;
  };
  struct compareYThenSerial {
    bool operator()(const Entity *a, const Entity *b) const;
  };
  typedef std::set<const Entity *, Entity::compareXThenSerial> byX_t;
  typedef std::set<const Entity *, Entity::compareYThenSerial> byY_t;

  Serial serial() const { return _serial; }
  void serial(Serial s) { _serial = s; }

  virtual void update(ms_t timeElapsed);
  // Add this entity to a list, for removal after all objects are updated.
  void markForRemoval();

  virtual void sendInfoToClient(const User &targetUser,
                                bool isNew = false) const = 0;

  virtual void writeToXML(XmlWriter &xw) const {}

  void changeType(const EntityType *newType,
                  bool shouldSkipConstruction = false);
  virtual void onSetType(bool shouldSkipConstruction = false);

  virtual void accountForOwnershipByUser(const User &owner) const {}

  Spawner *spawner() const { return _spawner; }
  void spawner(Spawner *p) { _spawner = p; }
  void separateFromSpawner();

  virtual bool shouldBePropagatedToClients() const { return true; }

  void removeOnTimer(ms_t &timer, ms_t timeElapsed);

  // Space
  const MapPoint &location() const { return _location; }
  void location(const MapPoint &loc, bool firstInsertion = false);
  virtual double legalMoveDistance(double requestedDistance,
                                   double timeElapsed) const;
  virtual bool shouldMoveWhereverRequested() const { return false; }
  bool teleportToValidLocationInCircle(const MapPoint &centre, double radius);
  void teleportTo(const MapPoint &destination);
  virtual Message teleportMessage(const MapPoint &destination) const;
  virtual void onTeleport() {}
  void changeDummyLocation(const MapPoint &loc) { _location = loc; }
  const MapRect collisionRect() const {
    return type()->collisionRect() + _location;
  }
  virtual bool collides() const;
  const TerrainList &allowedTerrain() const;
  virtual void onMove() {}

  // Combat
  Entity *target() const { return _target; }
  void target(Entity *p) { _target = p; }
  virtual void updateStats() {}  // Recalculate _stats based on any modifiers
  virtual ms_t timeToRemainAsCorpse() const = 0;
  ms_t corpseTime() const { return _corpseTime; }
  void corpseTime(ms_t time) { _corpseTime = time; }
  void setShorterCorpseTimerForFriendlyKill() { _corpseTime = 30000; }
  virtual bool shouldBeIgnoredByAIProximityAggro() const { return false; }
  virtual bool canBeAttackedBy(const User &user) const = 0;
  virtual bool canBeAttackedBy(const NPC &npc) const { return false; }
  virtual bool canAttack(const Entity &other) const { return false; }
  virtual px_t attackRange() const { return MELEE_RANGE; }
  CombatResult generateHitAgainst(const Entity &target, CombatType type,
                                  SpellSchool school, px_t range) const;
  virtual bool canBeHealedBySpell() const { return false; }
  static bool combatTypeCanHaveOutcome(CombatType type, CombatResult outcome,
                                       SpellSchool school, px_t range);
  virtual void sendGotHitMessageTo(const User &user) const;
  void regen(ms_t timeElapsed);
  virtual void scaleThreatAgainst(Entity &target, double multiplier) {}
  virtual bool isAttackingTarget() const {
    return true;
  }  // Assumption: entity has a target
  virtual SpellSchool school() const { return SpellSchool::PHYSICAL; }
  virtual Level level() const { return 0; }
  void resetAttackTimer() { _attackTimer = _stats.attackTime; }
  virtual double combatDamage() const { return 0; }
  virtual bool grantsXPOnDeath() const { return false; }
  virtual void onSuccessfulSpellcast(const std::string &id, const Spell &spell);

  const Buffs &buffs() const { return _buffs; }
  const Buffs &debuffs() const { return _debuffs; }
  std::vector<const Buff *> onHitBuffsAndDebuffs();
  std::vector<BuffType::ID> interruptibleBuffs() const;
  std::vector<BuffType::ID> buffsThatCancelOnOOE() const;
  void applyBuff(const BuffType &type, Entity &caster);
  void applyDebuff(const BuffType &type, Entity &caster,
                   ms_t customDuration = 0);
  virtual void loadBuff(const BuffType &type, ms_t timeRemaining);
  virtual void loadDebuff(const BuffType &type, ms_t timeRemaining);
  void removeBuff(Buff::ID id);
  void removeDebuff(Buff::ID id);
  void removeAllBuffsAndDebuffs();
  void removeInterruptibleBuffs();
  virtual void sendBuffMsg(const Buff::ID &buff) const;
  virtual void sendDebuffMsg(const Buff::ID &buff) const;
  virtual void sendLostBuffMsg(const Buff::ID &buff) const;
  virtual void sendLostDebuffMsg(const Buff::ID &buff) const;
  void updateBuffs(ms_t timeElapsed);

  CombatResult castSpell(const Spell &spell,
                         const std::string &supplementaryArg = {});

  const Stats &stats() const { return _stats; }
  void stats(const Stats &stats) { _stats = stats; }
  Hitpoints health() const { return _health; }
  Energy energy() const { return _energy; }
  virtual bool canBlock() const { return false; }
  bool isStunned() const { return _stats.stunned; }
  bool isSpellCoolingDown(const std::string &spell) const;
  const std::map<std::string, ms_t> &spellCooldowns() const {
    return _spellCooldowns;
  }
  void loadSpellCooldown(std::string id, ms_t remaining);

  void initStatsFromType();
  void fillHealthAndEnergy();
  void health(Hitpoints health) { _health = health; }  // TODO: Remove
  void energy(Energy energy) { _energy = energy; }     // TODO: Remove
  bool isDead() const { return _health == 0; }

  void kill() { reduceHealth(health()); }
  void reduceHealth(int damage);
  void reduceEnergy(int amount);
  void healBy(Hitpoints amount);
  bool isMissingHealth() const { return _health < _stats.maxHealth; }
  virtual void onHealthChange(){};  // Probably alerting relevant users.
  virtual void onEnergyChange();    // Probably alerting relevant users.
  virtual void onDeath();           // Anything that needs to happen upon death.
  virtual void onAttackedBy(
      Entity &attacker,
      Threat threat);  // If the entity needs to react to an attack.
  virtual void onKilled(Entity &victim) {}  // Upon this entity killing another
  // Inform user that this entity has missed its target with a ranged attack.
  virtual void sendRangedMissMessageTo(const User &userToInform) const {}
  // Inform user that this entity has hit its target with a ranged attack.
  virtual void sendRangedHitMessageTo(const User &userToInform) const {}
  // Any final checks immediately before the attack
  virtual void onOwnershipChange(){};
  virtual bool canAttack() { return true; }
  // Any reaction to a successful canAttack() check
  virtual void onCanAttack() {}
  virtual void onAttack() {}  // Any actions immediately before the attack
  virtual void broadcastDamagedMessage(Hitpoints amount) const {}
  virtual void broadcastHealedMessage(Hitpoints amount) const {}
  virtual int getLevelDifference(const User &user) const { return 0; }
  // const;
  virtual void sendAllLootToTaggers() const;
  void excludeFromPersistentState() { _excludedFromPersistentState = true; }
  void includeInPersistentState() { _excludedFromPersistentState = false; }
  bool excludedFromPersistentState() const {
    return _excludedFromPersistentState;
  }
  virtual void alertReactivelyTargetingUser(const User &targetingUser) const;

  void tellRelevantUsersAboutLootSlot(size_t slot) const;
  virtual ServerItem::Instance *getSlotToTakeFromAndSendErrors(
      size_t slotNum, const User &user) {
    return nullptr;
  }
  virtual void onOutOfRange(const Entity &rhs) const {
  }  // This will be called for both entities.
  virtual Message outOfRangeMessage() const { return Message(); };
  bool shouldAlwaysBeKnownToUser(const User &user) const;

  const Loot &loot() const;

  /*
  Determine whether the proposed new location is legal, considering movement
  speed and time elapsed, and checking for collisions. Set location to the new,
  legal location.
  */
  enum ClientLocationUpdateCase { OnServerCorrection, AlwaysSendUpdate };
  enum StraightLineMoveResult {
    MOVED_FREELY,
    MOVED_INTO_OBSTACLE,
    DID_NOT_MOVE,
    TELEPORTED_NEARBY
  };
  StraightLineMoveResult moveLegallyTowards(
      const MapPoint &requestedDest,
      ClientLocationUpdateCase whenToSendClientHisLocation =
          OnServerCorrection);

  virtual bool areOverlapsAllowedWith(const Entity &rhs) const;

  Permissions permissions;
  Gatherable gatherable;
  Transformation transformation;
  Tagger tagger;  // The user who gets credit for killing this.

 protected:
  // void type(const EntityType *type) { _type = type; }
  std::shared_ptr<Loot> _loot;
  void resetLocationUpdateTimer() {
    _lastLocUpdate = SDL_GetTicks();
  }  // To be called when movement starts
  static const px_t MELEE_RANGE;

 private:
  const EntityType *_type{nullptr};

  bool _excludedFromPersistentState{false};

  Spawner *_spawner{nullptr};  // The Spawner that created this entity, if any.

  // Space
  Serial _serial;
  MapPoint _location;
  ms_t _lastLocUpdate;  // Time that the location was last updated; used to
                        // determine max distance

  // Combat
  Stats _stats;  // Memoized stats, after gear, buffs, etc.  Calculated with
                 // updateStats();
  Hitpoints _health;
  Energy _energy;
  ms_t _attackTimer{0};
  Entity *_target{nullptr};
  ms_t _corpseTime{0};  // How much longer this entity should exist as a corpse.
  void startCorpseTimer();
  Buffs _buffs, _debuffs;

 protected:
  std::map<std::string, ms_t> _spellCooldowns;  // >0 = cooling down

 private:
  ms_t _timeSinceRegen = 0;

  friend class Dummy;
};

class Dummy : public Entity {
 public:
  static Dummy FromSerial(Serial serial) { return Dummy(serial); }
  static Dummy Location(const MapPoint &loc) { return Dummy(loc); }
  static Dummy Location(double x, double y) { return Dummy({x, y}); }

 private:
  friend class Entity;
  Dummy(Serial serial) : Entity(serial) {}
  Dummy(const MapPoint &loc) : Entity(loc) {}

  // Necessary overrides to make this a concrete class
  char classTag() const override { return 'd'; }
  void sendInfoToClient(const User &targetUser,
                        bool isNew = false) const override {}
  ms_t timeToRemainAsCorpse() const override { return 0; }
  bool canBeAttackedBy(const User &) const override { return false; }
  static Stats _stats;
};

double distance(const Entity &a, const Entity &b);

#endif
