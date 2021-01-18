#include "Entity.h"

#include <algorithm>

#include "../util.h"
#include "Groups.h"
#include "Server.h"
#include "Spawner.h"
#include "Spell.h"
//#include "User.h"

const px_t Entity::MELEE_RANGE = Podes{4}.toPixels();

Stats Dummy::_stats{};

Entity::Entity(const EntityType *type, const MapPoint &loc)
    : _type(type),
      _serial(Serial::Generate()),

      _location(loc),
      _lastLocUpdate(SDL_GetTicks()),

      permissions(*this),
      gatherable(*this),
      transformation(*this) {
  initStatsFromType();
}

Entity::Entity(Serial serial)
    :  // For set/map lookup ONLY
      _serial(serial),
      permissions(*this),
      gatherable(*this),
      transformation(*this) {}

Entity::Entity(const MapPoint &loc)
    :  // For set/map lookup ONLY
      _location(loc),
      permissions(*this),
      gatherable(*this),
      transformation(*this) {}

Entity::~Entity() {
  if (_spawner) _spawner->scheduleSpawn();
}

bool Entity::compareSerial::operator()(const Entity *a, const Entity *b) const {
  return a->_serial < b->_serial;
}

bool Entity::compareXThenSerial::operator()(const Entity *a,
                                            const Entity *b) const {
  if (a->_location.x != b->_location.x) return a->_location.x < b->_location.x;
  return a->_serial < b->_serial;
}

bool Entity::compareYThenSerial::operator()(const Entity *a,
                                            const Entity *b) const {
  if (a->_location.y != b->_location.y) return a->_location.y < b->_location.y;
  return a->_serial < b->_serial;
}

void Entity::markForRemoval() {
  Server::_instance->_entitiesToRemove.push_back(this);
}

void Entity::changeType(const EntityType *newType,
                        bool shouldSkipConstruction) {
  if (!newType) {
    SERVER_ERROR("Trying to set object type to null");
    return;
  }

  auto &server = Server::instance();

  if (classTag() == 'n')
    _type = dynamic_cast<const NPCType *>(newType);
  else
    _type = newType;
  server.forceAllToUntarget(*this);

  gatherable.removeAllGatheringUsers();

  onSetType(shouldSkipConstruction);

  // Inform nearby users
  for (const User *user : server.findUsersInArea(location()))
    sendInfoToClient(*user);
  // Inform owner
  for (const auto &owner : permissions.ownerAsUsernames())
    server.sendMessageIfOnline(
        owner, {SV_OBJECT_INFO,
                makeArgs(serial(), location().x, location().y, type()->id())});
}

void Entity::onSetType(bool shouldSkipConstruction) {
  gatherable.populateContents();
  transformation.initialise();
}

CombatResult Entity::generateHitAgainst(const Entity &target, CombatType type,
                                        SpellSchool school, px_t range) const {
  auto levelDiff = target.level() - level();
  auto modifierFromLevelDiff = levelDiff * .03;
  auto roll = randDouble();
  // Miss
  const auto BASE_MISS_CHANCE = 0.1;
  auto missChance =
      BASE_MISS_CHANCE - stats().hit.asChance() + modifierFromLevelDiff;
  missChance = max(missChance, 0);
  if (combatTypeCanHaveOutcome(type, MISS, school, range)) {
    if (roll < missChance) return MISS;
    roll -= missChance;
  }
  // Dodge
  auto dodgeChance = target.stats().dodge.asChance() + modifierFromLevelDiff;
  dodgeChance = max(dodgeChance, 0);
  if (combatTypeCanHaveOutcome(type, DODGE, school, range)) {
    if (roll < dodgeChance) return DODGE;
    roll -= dodgeChance;
  }
  // Block
  auto blockChance = target.stats().block.asChance() + modifierFromLevelDiff;
  blockChance = max(blockChance, 0);
  if (target.canBlock() &&
      combatTypeCanHaveOutcome(type, BLOCK, school, range)) {
    if (roll < blockChance) return BLOCK;
    roll -= blockChance;
  }
  // Crit
  auto critChance = stats().crit.asChance() -
                    target.stats().critResist.asChance() -
                    modifierFromLevelDiff;
  critChance = max(critChance, 0);
  if (critChance > 0 && combatTypeCanHaveOutcome(type, CRIT, school, range)) {
    if (roll < critChance) return CRIT;
    roll -= critChance;
  }
  return HIT;
}

bool Entity::combatTypeCanHaveOutcome(CombatType type, CombatResult outcome,
                                      SpellSchool school, px_t range) {
  /*
              Miss    Dodge   Block   Crit    Hit
  Spell               X       X
  Physical
  Ranged              X
  Heal        X       X       X
  Debuff              X       X       X
  */
  if (outcome == HIT) return true;
  if (outcome == CRIT) return type != DEBUFF;
  if (outcome == MISS) return type != HEAL;
  const auto isRanged = range > Podes::MELEE_RANGE;

  // Remaining: dodge and block
  if (type != DAMAGE || school.isMagic()) return false;
  if (outcome == DODGE && isRanged) return false;
  return true;
}

void Entity::sendGotHitMessageTo(const User &user) const {
  user.sendMessage({SV_ENTITY_WAS_HIT, serial()});
}

void Entity::loadSpellCooldown(std::string id, ms_t remaining) {
  _spellCooldowns[id] = remaining;
}

void Entity::initStatsFromType() {
  _stats = _type->baseStats();
  _health = _stats.maxHealth;
  _energy = _stats.maxEnergy;
}

void Entity::fillHealthAndEnergy() {
  _health = _stats.maxHealth;
  onHealthChange();
  _energy = _stats.maxEnergy;
  onEnergyChange();
}

void Entity::reduceHealth(int damage) {
  if (damage == 0) return;
  if (damage >= static_cast<int>(_health)) {
    startCorpseTimer();
    _health = 0;
    onHealthChange();
    onDeath();
  } else {
    _health -= damage;
    if (_health > this->_stats.maxHealth) {
      Server::debug()("reduceHealth(): Entity has too much health: "s +
                          toString(_health) + "/"s +
                          toString(this->_stats.maxHealth),
                      Color::CHAT_ERROR);
      _health = this->_stats.maxHealth;
    }
    onHealthChange();
  }
  broadcastDamagedMessage(damage);
}

void Entity::reduceEnergy(int amount) {
  if (amount == 0) return;
  if (amount > static_cast<int>(_energy)) amount = _energy;
  _energy -= amount;
  onEnergyChange();
}

void Entity::healBy(Hitpoints amount) {
  auto newHealth = min(health() + amount, _stats.maxHealth);
  _health = newHealth;
  onHealthChange();
  broadcastHealedMessage(amount);
}

void Entity::update(ms_t timeElapsed) {
  // Corpse timer
  if (isDead()) {
    if (_corpseTime > timeElapsed)
      _corpseTime -= timeElapsed;
    else
      markForRemoval();
    return;
  }

  regen(timeElapsed);
  updateBuffs(timeElapsed);

  // Spell cooldowns
  for (auto &pair : _spellCooldowns) {
    auto &cooldown = pair.second;
    if (cooldown < timeElapsed)
      cooldown = 0;
    else
      cooldown -= timeElapsed;
  }

  transformation.update(timeElapsed);

  // The remainder of this function deals with combat.
  if (_attackTimer > timeElapsed)
    _attackTimer -= timeElapsed;
  else
    _attackTimer = 0;

  if (isStunned()) return;

  auto pTarget = target();
  if (!pTarget) return;
  if (!isAttackingTarget()) return;
  if (_attackTimer > 0) return;

  if (pTarget->isDead()) return;

  if (!canAttack()) return;
  onCanAttack();

  if (combatDamage() == 0) return;

  // Check if within range
  if (distance(*this, *pTarget) > attackRange()) return;

  resetAttackTimer();

  MapPoint locus = midpoint(location(), pTarget->location());

  auto outcome = generateHitAgainst(*pTarget, DAMAGE, school(), attackRange());

  const Server &server = Server::instance();
  auto usersToInform = server.findUsersInArea(locus);
  auto targetLoc = makeArgs(pTarget->location().x, pTarget->location().y);

  switch (outcome) {
    // These cases return
    case MISS:
      for (auto user : usersToInform) {
        user->sendMessage({SV_SHOW_MISS_AT, targetLoc});
        if (attackRange() > MELEE_RANGE) sendRangedMissMessageTo(*user);
      }
      return;
    case DODGE:
      for (auto user : usersToInform) {
        user->sendMessage({SV_SHOW_DODGE_AT, targetLoc});
        if (attackRange() > MELEE_RANGE) sendRangedMissMessageTo(*user);
      }
      return;

    // These cases continue on
    case CRIT:
      for (auto user : usersToInform)
        user->sendMessage({SV_SHOW_CRIT_AT, targetLoc});
      break;
    case BLOCK:
      for (auto user : usersToInform)
        user->sendMessage({SV_SHOW_BLOCK_AT, targetLoc});
      break;
  }

  // Send ranged message if hit.  This tells the client to create a projectile.
  // This must be done before the damage, because if the damage kills the target
  // and the attacker subsequently untargets it, then this message will not be
  // sent.
  if (attackRange() > MELEE_RANGE)
    for (auto user : usersToInform) sendRangedHitMessageTo(*user);

  // Actually do the damage.
  auto rawDamage = combatDamage();
  if (outcome == CRIT) rawDamage *= 2;

  auto resistance = pTarget->_stats.resistanceByType(school());
  resistance = resistance.modifyByLevelDiff(level(), pTarget->level());
  rawDamage = resistance.applyTo(rawDamage);

  auto damage = SpellEffect::chooseRandomSpellMagnitude(rawDamage);

  if (outcome == BLOCK) {
    const auto blockValue = pTarget->_stats.blockValue.effectiveValue();
    if (blockValue >= damage)
      damage = 0;
    else
      damage -= blockValue;
  }

  // This is called after damage is determined, as it may result in the weapon
  // breaking.
  onAttack();

  // Give target opportunity to react.  This includes tagging, and so must be
  // done before damage (and possible death)
  pTarget->onAttackedBy(*this, damage);

  pTarget->reduceHealth(damage);

  // Alert nearby clients.  This must be done after the damage, so that the
  // client knows whether to play a hit sound or a death sound.
  MessageCode msgCode;
  std::string args;
  const auto attackerIsAPlayer = classTag() == 'u';
  const auto defenderIsAPlayer = pTarget->classTag() == 'u';
  if (attackerIsAPlayer && defenderIsAPlayer) {
    msgCode = SV_PLAYER_HIT_PLAYER;
    args = makeArgs(dynamic_cast<const User *>(this)->name(),
                    dynamic_cast<const User *>(pTarget)->name());
  } else if (attackerIsAPlayer) {
    msgCode = SV_PLAYER_HIT_ENTITY;
    args =
        makeArgs(dynamic_cast<const User *>(this)->name(), pTarget->serial());
  } else if (defenderIsAPlayer) {
    msgCode = SV_ENTITY_HIT_PLAYER;
    args = makeArgs(serial(), dynamic_cast<const User *>(pTarget)->name());
  } else {
    msgCode = SV_ENTITY_HIT_ENTITY;
    args = makeArgs(serial(), pTarget->serial());
  }
  for (auto user : usersToInform) user->sendMessage({msgCode, args});
}

void Entity::updateBuffs(ms_t timeElapsed) {
  auto expiredBuffs = std::set<Buff::ID>{};
  for (auto &buff : _buffs) {
    buff.update(timeElapsed);
    if (isDead()) return;
    if (buff.hasExpired()) expiredBuffs.insert(buff.type());
  }

  auto expiredDebuffs = std::set<Buff::ID>{};
  for (auto &debuff : _debuffs) {
    debuff.update(timeElapsed);
    if (isDead()) return;
    if (debuff.hasExpired()) expiredDebuffs.insert(debuff.type());
  }

  for (auto buffID : expiredBuffs) removeBuff(buffID);
  for (auto debuffID : expiredDebuffs) removeDebuff(debuffID);

  auto aBuffHasExpired = !expiredBuffs.empty() || !expiredDebuffs.empty();
  if (aBuffHasExpired) updateStats();
}

bool Entity::isSpellCoolingDown(const std::string &spell) const {
  auto it = _spellCooldowns.find(spell);
  if (it == _spellCooldowns.end()) return false;
  return it->second > 0;
}

CombatResult Entity::castSpell(const Spell &spell,
                               const std::string &supplementaryArg) {
  const Server &server = Server::instance();

  auto usersNearCaster = server.findUsersInArea(location());

  const auto &effect = spell.effect();
  auto targets = std::set<Entity *>{};
  if (effect.isAoE()) {
    targets = server.findEntitiesInArea(location(), effect.range());
    auto nearbyUsers = server.findUsersInArea(location(), effect.range());
    for (auto *user : nearbyUsers) targets.insert(user);
  } else {
    auto target = this->target();
    if (!target)
      target = this;
    else if (spell.canCastOnlyOnSelf())
      target = this;

    else if (canAttack(*_target) && !spell.canTarget(Spell::ENEMY))
      target = this;

    targets.insert(target);
  }

  if (effect.isAoE()) reduceEnergy(spell.cost());

  // Note: this will be set for each target.  Return value will vary based on
  // target order, if >1. When the return value was added to this function, it
  // was used only for eating food; i.e., a single target. Its purpose was to
  // skip consuming the food if the spell failed.
  auto outcome = CombatResult{};
  for (auto target : targets) {
    // Cull target if type is restricted
    const auto spellIsAllowedToTargetThisType =
        !spell.isTargetingRestrictedToSpecificNPC() ||
        target->type()->id() == spell.onlyAllowedNPCTarget();
    if (!spellIsAllowedToTargetThisType) continue;

    outcome = spell.performAction(*this, *target, supplementaryArg);
    if (outcome == FAIL) {
      Server::debug()("Spell "s + spell.id() + " failed."s, Color::CHAT_ERROR);
      continue;
    }

    if (!effect.isAoE())  // The spell succeeded, and there should be only one
                          // iteration.
      reduceEnergy(spell.cost());

    // Broadcast spellcast
    auto spellHit = bool{};
    switch (outcome) {
      case MISS:
      case DODGE:
        spellHit = false;
        break;
      case HIT:
      case CRIT:
      case BLOCK:
        spellHit = true;
        break;
      default:
        SERVER_ERROR("Invalid spell outcome");
    }
    auto msgCode = spellHit ? SV_SPELL_HIT : SV_SPELL_MISS;
    const auto &src = location(), &dst = target->location();
    auto args = makeArgs(spell.id(), src.x, src.y, dst.x, dst.y);

    auto usersToAlert = server.findUsersInArea(dst);
    usersToAlert.insert(usersNearCaster.begin(), usersNearCaster.end());
    for (auto user : usersToAlert) {
      user->sendMessage({msgCode, args});
      if (spellHit && spell.shouldPlayDefenseSound())
        target->sendGotHitMessageTo(*user);

      // Show notable outcomes
      switch (outcome) {
        case MISS:
          user->sendMessage({SV_SHOW_MISS_AT, makeArgs(dst.x, dst.y)});
          break;
        case DODGE:
          user->sendMessage({SV_SHOW_DODGE_AT, makeArgs(dst.x, dst.y)});
          break;
        case BLOCK:
          user->sendMessage({SV_SHOW_BLOCK_AT, makeArgs(dst.x, dst.y)});
          break;
        case CRIT:
          user->sendMessage({SV_SHOW_CRIT_AT, makeArgs(dst.x, dst.y)});
          break;
      }
    }
  }

  if (effect.isAoE()) outcome = HIT;

  if (outcome != FAIL) onSuccessfulSpellcast(spell.id(), spell);

  return outcome;
}

void Entity::onEnergyChange() {
  if (energy() > 0) return;

  // Remove cancel-on-OOE buffs
  for (auto buff : buffsThatCancelOnOOE()) removeBuff(buff);
}

void Entity::onDeath() {
  auto &server = Server::instance();
  removeAllBuffsAndDebuffs();

  if (tagger.asUser()) {
    auto taggersGroup = server.groups->getUsersGroup(tagger.username());
    for (auto groupMember : taggersGroup) {
      auto asUser = server.getUserByName(groupMember);
      if (!asUser) continue;
      asUser->onKilled(*this);
    }
  }

  if (_spawner) {
    _spawner->scheduleSpawn();
    _spawner = nullptr;
  }
}

void Entity::onAttackedBy(Entity &attacker, Threat threat) {
  // Tag target
  if (attacker.classTag() == 'u') {
    auto &attackerAsUser = dynamic_cast<User &>(attacker);
    if (!tagger) tagger = attackerAsUser;
  }

  // Proc on-hit buffs
  for (const auto *buff : onHitBuffsAndDebuffs()) {
    buff->proc(&attacker);
  }

  // Remove interruptible buffs
  for (auto buff : interruptibleBuffs()) removeBuff(buff);
}

void Entity::startCorpseTimer() { _corpseTime = timeToRemainAsCorpse(); }

void Entity::location(const MapPoint &newLoc, bool firstInsertion) {
  Server &server = *Server::_instance;

  const User *selfAsUser = nullptr;
  if (classTag() == 'u') selfAsUser = dynamic_cast<const User *>(this);

  MapPoint oldLoc = _location;

  auto xChanged = newLoc.x != oldLoc.x;
  auto yChanged = newLoc.y != oldLoc.y;

  if (!firstInsertion) {
    // Remove from location-indexed trees
    if (server._entitiesByX.size() != server._entitiesByY.size())
      SERVER_ERROR(
          "x-indexed and y-indexed entities lists have different sizes");

    if (classTag() == 'u') {
      if (xChanged) {
        auto numRemoved = server._usersByX.erase(selfAsUser);
        if (numRemoved != 1)
          SERVER_ERROR("Unexpected number of entities removed");
      }
      if (yChanged) {
        auto numRemoved = server._usersByY.erase(selfAsUser);
        if (numRemoved != 1)
          SERVER_ERROR("Unexpected number of entities removed");
      }
    }
    if (xChanged) {
      auto numRemoved = server._entitiesByX.erase(this);
      if (numRemoved != 1)
        SERVER_ERROR("Unexpected number of entities removed");
    }
    if (yChanged) {
      auto numRemoved = server._entitiesByY.erase(this);
      if (numRemoved != 1)
        SERVER_ERROR("Unexpected number of entities removed");
    }
  }

  _location = newLoc;

  // Re-insert into location-indexed trees
  if (classTag() == 'u') {
    if (xChanged) server._usersByX.insert(selfAsUser);
    if (yChanged) server._usersByY.insert(selfAsUser);
    if (server._usersByX.size() != server._usersByY.size())
      SERVER_ERROR("x-indexed users lists have different sizes");
  }
  if (xChanged) server._entitiesByX.insert(this);
  if (yChanged) server._entitiesByY.insert(this);
  if (server._entitiesByX.size() != server._entitiesByY.size())
    SERVER_ERROR("x-indexed and y-indexed entities lists have different sizes");

  // Move to a different collision chunk if needed
  auto &oldCollisionChunk = server.getCollisionChunk(oldLoc),
       &newCollisionChunk = server.getCollisionChunk(_location);
  if (firstInsertion || &oldCollisionChunk != &newCollisionChunk) {
    oldCollisionChunk.removeEntity(_serial);
    newCollisionChunk.addEntity(this);
  }

  onMove();
}

double Entity::legalMoveDistance(double requestedDistance,
                                 double timeElapsed) const {
  auto maxLegalDistance = timeElapsed / 1000.0 * stats().speed;
  return min(maxLegalDistance, requestedDistance);
}

bool Entity::teleportToValidLocationInCircle(const MapPoint &centre,
                                             double maxRadius) {
  auto &server = Server::instance();

  auto attempts = 100;
  while (attempts-- > 0) {
    auto angle = randDouble() * 2 * PI;
    auto radius = sqrt(randDouble()) * maxRadius;
    auto dX = cos(angle) * radius;
    auto dY = sin(angle) * radius;

    const auto proposedLocation = centre + MapPoint{dX, dY};
    if (server.isLocationValid(proposedLocation, *this)) {
      teleportTo(proposedLocation);
      return true;
    };
  }

  return false;
}

void Entity::teleportTo(const MapPoint &destination) {
  const auto &server = Server::instance();
  auto startingLocation = location();

  location(destination);

  auto message = teleportMessage(destination);
  server.broadcastToArea(startingLocation, message);
  server.broadcastToArea(destination, message);

  onTeleport();
}

Message Entity::teleportMessage(const MapPoint &destination) const {
  return {SV_ENTITY_LOCATION_INSTANT,
          makeArgs(serial(), destination.x, destination.y)};
}

bool Entity::collides() const {
  if (isDead()) return false;
  return (type()->collides());
}

const TerrainList &Entity::allowedTerrain() const {
  for (const auto &buff : buffs()) {
    auto newTerrainList = buff.changesAllowedTerrain();
    if (!newTerrainList) continue;
    return *newTerrainList;
  }

  return _type->allowedTerrain();
}

void Entity::sendAllLootToTaggers() const {
  auto &server = Server::instance();

  if (!tagger) return;
  auto taggersGroup = server.groups->getUsersGroup(tagger.username());
  for (auto memberName : taggersGroup) {
    auto asUser = server.getUserByName(memberName);
    if (!asUser) continue;
    for (auto i = 0; i != loot().size(); ++i)
      loot().sendSingleSlotToUser(*asUser, serial(), i);
  }
}

void Entity::separateFromSpawner() {
  if (!_spawner) return;
  _spawner->scheduleSpawn();  // To replace this
  _spawner = nullptr;
}

void Entity::alertReactivelyTargetingUser(const User &targetingUser) const {
  targetingUser.sendMessage({SV_YOU_ARE_ATTACKING_ENTITY, serial()});
}

void Entity::tellRelevantUsersAboutLootSlot(size_t slot) const {
  // Ultimately this might need a call to findUsersInArea().  For now, there's
  // only one tagger and only he should get this information.
  if (!tagger.asUser()) return;

  _loot->sendSingleSlotToUser(*tagger.asUser(), serial(), slot);
}

bool Entity::shouldAlwaysBeKnownToUser(const User &user) const {
  if (permissions.isOwnedByPlayer(user.name())) return true;
  const Server &server = *Server::_instance;
  const auto &city = server.cities().getPlayerCity(user.name());
  if (!city.empty() && permissions.isOwnedByCity(city)) return true;
  return false;
}

const Loot &Entity::loot() const {
  static Loot dummy{};

  if (_loot == nullptr) {
    SERVER_ERROR("_loot is null");
    return dummy;
  }

  return *_loot;
}

void Entity::onSuccessfulSpellcast(const std::string &id, const Spell &spell) {
  _spellCooldowns[spell.id()] = spell.cooldown();
}

std::vector<const Buff *> Entity::onHitBuffsAndDebuffs() {
  auto v = std::vector<const Buff *>{};
  for (const auto &buff : _buffs)
    if (buff.hasEffectOnHit()) v.push_back(&buff);
  for (const auto &debuff : _debuffs)
    if (debuff.hasEffectOnHit()) v.push_back(&debuff);
  return v;
}

std::vector<BuffType::ID> Entity::interruptibleBuffs() const {
  auto ret = std::vector<BuffType::ID>{};
  for (const auto &buff : _buffs) {
    if (buff.canBeInterrupted()) ret.push_back(buff.type());
  }
  return ret;
}

std::vector<BuffType::ID> Entity::buffsThatCancelOnOOE() const {
  auto ret = std::vector<BuffType::ID>{};
  for (const auto &buff : _buffs) {
    if (buff.cancelsOnOOE()) ret.push_back(buff.type());
  }
  return ret;
}

void Entity::applyBuff(const BuffType &type, Entity &caster) {
  auto newBuff = Buff{type, *this, caster};

  // Check whether it doesn't stack with something else
  for (auto &buff : _buffs) {
    if (buff.doesntStackWith(type)) {
      return;
    }
  }

  // Check for duplicates
  auto buffWasReapplied = false;
  for (auto &buff : _buffs) {
    if (buff.hasSameType(newBuff)) {
      buff = newBuff;
      buffWasReapplied = true;
      break;
    }
  }

  if (!buffWasReapplied) _buffs.push_back(newBuff);

  sendBuffMsg(type.id());

  if (!buffWasReapplied) {
    updateStats();

    if (classTag() == 'u' && type.changesAllowedTerrain()) {
      auto &user = dynamic_cast<User &>(*this);
      user.onTerrainListChange(type.changesAllowedTerrain()->id());
    }
  }
}

void Entity::applyDebuff(const BuffType &type, Entity &caster) {
  auto newDebuff = Buff{type, *this, caster};

  // Check for duplicates
  auto debuffWasReapplied = false;
  for (auto &debuff : _debuffs) {
    if (debuff.hasSameType(newDebuff)) {
      debuff = newDebuff;
      debuffWasReapplied = true;
      break;
    }
  }

  if (!debuffWasReapplied) _debuffs.push_back(newDebuff);

  sendDebuffMsg(type.id());

  if (!debuffWasReapplied) {
    updateStats();

    if (classTag() == 'u' && type.changesAllowedTerrain()) {
      auto &user = dynamic_cast<User &>(*this);
      user.onTerrainListChange(type.changesAllowedTerrain()->id());
    }
  }
}

void Entity::loadBuff(const BuffType &type, ms_t timeRemaining) {
  auto newBuff = Buff{type, *this, timeRemaining};

  _buffs.push_back(newBuff);
  sendBuffMsg(type.id());

  updateStats();

  if (classTag() == 'u' && type.changesAllowedTerrain()) {
    auto &user = dynamic_cast<User &>(*this);
    user.onTerrainListChange(type.changesAllowedTerrain()->id());
  }
}

void Entity::loadDebuff(const BuffType &type, ms_t timeRemaining) {
  auto newDebuff = Buff{type, *this, timeRemaining};

  _debuffs.push_back(newDebuff);
  sendDebuffMsg(type.id());

  updateStats();

  if (classTag() == 'u' && type.changesAllowedTerrain()) {
    auto &user = dynamic_cast<User &>(*this);
    user.onTerrainListChange(type.changesAllowedTerrain()->id());
  }
}

void Entity::removeBuff(Buff::ID id) {
  for (auto it = _buffs.begin(); it != _buffs.end(); ++it)
    if (it->type() == id) {
      const auto changesAllowedTerrain = it->changesAllowedTerrain();

      _buffs.erase(it);
      updateStats();

      sendLostBuffMsg(id);

      if (classTag() == 'u' && changesAllowedTerrain) {
        auto &user = dynamic_cast<User &>(*this);
        user.onTerrainListChange(type()->allowedTerrain().id());
      }

      return;
    }
}

void Entity::removeDebuff(Buff::ID id) {
  for (auto it = _debuffs.begin(); it != _debuffs.end(); ++it)
    if (it->type() == id) {
      const auto changesAllowedTerrain = it->changesAllowedTerrain();

      _debuffs.erase(it);
      updateStats();

      sendLostDebuffMsg(id);

      if (classTag() == 'u' && changesAllowedTerrain) {
        auto &user = dynamic_cast<User &>(*this);
        user.onTerrainListChange(type()->allowedTerrain().id());
      }

      return;
    }
}

void Entity::removeAllBuffsAndDebuffs() {
  auto buffIDs = std::set<std::string>{};
  for (const auto &buff : buffs()) buffIDs.insert(buff.type());
  for (const auto &buffID : buffIDs) removeBuff(buffID);

  auto debuffIDs = std::set<std::string>{};
  for (const auto &debuff : debuffs()) debuffIDs.insert(debuff.type());
  for (const auto &debuffID : debuffIDs) removeDebuff(debuffID);
}

void Entity::sendBuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(_location,
                         {SV_ENTITY_GOT_BUFF, makeArgs(_serial, buff)});
}

void Entity::sendDebuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(_location,
                         {SV_ENTITY_GOT_DEBUFF, makeArgs(_serial, buff)});
}

void Entity::sendLostBuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(_location,
                         {SV_ENTITY_LOST_BUFF, makeArgs(_serial, buff)});
}

void Entity::sendLostDebuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(_location,
                         {SV_ENTITY_LOST_DEBUFF, makeArgs(_serial, buff)});
}

void Entity::regen(ms_t timeElapsed) {
  // Regen
  _timeSinceRegen += timeElapsed;
  if (_timeSinceRegen < 1000) return;

  _timeSinceRegen -= 1000;

  if (stats().hps.hasValue()) {
    auto oldHealth = health();
    int rawNewHealth = health() + stats().hps.getNextWholeAmount();
    if (rawNewHealth < 0)
      health(0);
    else if (0 + rawNewHealth > static_cast<int>(stats().maxHealth) + 0)
      health(stats().maxHealth);
    else
      health(rawNewHealth);

    if (health() != oldHealth) onHealthChange();

    if (isDead()) onDeath();
  }

  if (stats().eps.hasValue()) {
    auto oldEnergy = energy();
    int rawNewEnergy = energy() + stats().eps.getNextWholeAmount();
    if (rawNewEnergy < 0)
      energy(0);
    else if (rawNewEnergy > static_cast<int>(stats().maxEnergy))
      energy(stats().maxEnergy);
    else
      energy(rawNewEnergy);

    if (energy() != oldEnergy) onEnergyChange();
  }
}

double distance(const Entity &a, const Entity &b) {
  return distance(a.collisionRect(), b.collisionRect());
}
