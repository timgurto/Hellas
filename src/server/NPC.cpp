#include "NPC.h"

#include "Server.h"
#include "User.h"

NPC::NPC(const NPCType *type, const MapPoint &loc)
    : Entity(type, loc),
      QuestNode(*type, serial()),
      _level(type->level()),
      _state(IDLE) {
  _loot.reset(new Loot);
}

void NPC::update(ms_t timeElapsed) {
  if (health() > 0) {
    processAI(timeElapsed);  // May call Entity::update()
  }

  Entity::update(timeElapsed);
}

bool NPC::canBeAttackedBy(const User &user) const {
  if (!npcType()->canBeAttacked()) return false;
  return true;
}

CombatResult NPC::generateHitAgainst(const Entity &target, CombatType type,
                                     SpellSchool school, px_t range) const {
  const auto MISS_CHANCE = Percentage{10};

  auto levelDiff = target.level() - level();
  auto modifierFromLevelDiff = levelDiff * 3;

  auto roll = rand() % 100;

  // Miss
  auto missChance = MISS_CHANCE + modifierFromLevelDiff;
  missChance = max(missChance, 0);
  if (combatTypeCanHaveOutcome(type, MISS, school, range)) {
    if (roll < missChance) return MISS;
    roll -= missChance;
  }

  // Dodge
  auto dodgeChance = target.stats().dodge + modifierFromLevelDiff;
  dodgeChance = max(dodgeChance, 0);
  if (combatTypeCanHaveOutcome(type, DODGE, school, range)) {
    if (roll < dodgeChance) return DODGE;
    roll -= dodgeChance;
  }

  // Block
  auto blockChance = target.stats().block + modifierFromLevelDiff;
  blockChance = max(blockChance, 0);
  if (target.canBlock() &&
      combatTypeCanHaveOutcome(type, BLOCK, school, range)) {
    if (roll < blockChance) return BLOCK;
    roll -= blockChance;
  }

  // Crit
  auto critChance = target.stats().critResist - modifierFromLevelDiff;
  critChance = max(critChance, 0);
  if (critChance > 0 && combatTypeCanHaveOutcome(type, CRIT, school, range)) {
    if (roll < critChance) return CRIT;
    roll -= critChance;
  }

  return HIT;
}

void NPC::scaleThreatAgainst(Entity &target, double multiplier) {
  _threatTable.scaleThreat(target, multiplier);
}

void NPC::makeAwareOf(User &user) {
  if (!npcType()->attacksNearby()) return;

  // For when an aggressive NPC begins combat
  if (_threatTable.isEmpty()) _timeEngaged = SDL_GetTicks();

  _threatTable.makeAwareOf(user);
  makeNearbyNPCsAwareOf(user);
  user.putInCombat();
}

bool NPC::isAwareOf(User &user) const { return _threatTable.isAwareOf(user); }

void NPC::makeNearbyNPCsAwareOf(User &user) {
  const Server &server = *Server::_instance;

  // Skip those already aware, otherwise we'd get infinite loops
  for (auto entity : server._entities) {
    auto npc = dynamic_cast<NPC *>(entity);
    if (!npc) continue;
    if (npc->isAwareOf(user)) continue;

    const auto CHAIN_PULL_DISTANCE = Podes{10}.toPixels();
    if (distance(location(), npc->location()) <= CHAIN_PULL_DISTANCE)
      npc->makeAwareOf(user);
  }
}

void NPC::addThreat(User &attacker, Threat amount) {
  makeAwareOf(attacker);
  _threatTable.addThreat(attacker, amount);
}

Message NPC::outOfRangeMessage() const {
  return Message(SV_OBJECT_OUT_OF_RANGE, makeArgs(serial()));
}

void NPC::onHealthChange() {
  const Server &server = *Server::_instance;
  for (const User *user : server.findUsersInArea(location()))
    user->sendMessage(SV_ENTITY_HEALTH, makeArgs(serial(), health()));
}

void NPC::onDeath() {
  Server &server = *Server::_instance;
  server.forceAllToUntarget(*this);

  if (_timeEngaged > 0) {
    auto timeNow = SDL_GetTicks();
    auto timeToKill = timeNow - _timeEngaged;

    auto killerClass = ""s;
    auto killerLevel = 0;
    if (tagger() != nullptr) {
      killerClass = tagger()->getClass().type().id();
      killerLevel = tagger()->level();
    }

    auto of = std::ofstream{"kills.log", std::ios_base::app};
    of << type()->id()                  // NPC ID
       << "," << killerClass            // Killer's class
       << "," << _threatTable.size()    // Entities in threat table
       << "," << level()                // NPC level
       << "," << killerLevel            // Killer's level
       << "," << timeToKill             // Time between engagement and death
       << "," << npcType()->isRanged()  // Whether NPC is ranged
       << std::endl;

  } else {
    SERVER_ERROR("NPC killed without having been engaged by a user");
  }

  npcType()->lootTable().instantiate(*_loot, tagger());
  if (!_loot->empty()) sendAllLootToTagger();

  /*
  Schedule a respawn, if this NPC came from a spawner.
  For non-NPCs, this happens in onRemove().  The object's _spawner is cleared
  afterwards to avoid onRemove() from doing so here.
  */
  if (spawner() != nullptr) {
    spawner()->scheduleSpawn();
    spawner(nullptr);
  }

  Entity::onDeath();
}

void NPC::onAttackedBy(Entity &attacker, Threat threat) {
  if (attacker.classTag() == 'u') {
    if (_threatTable.isEmpty()) _timeEngaged = SDL_GetTicks();
    addThreat(dynamic_cast<User &>(attacker), threat);
  }

  Entity::onAttackedBy(attacker, threat);
}

px_t NPC::attackRange() const {
  if (npcType()->isRanged()) return Podes{20}.toPixels();
  return MELEE_RANGE;
}

void NPC::sendRangedHitMessageTo(const User &userToInform) const {
  if (!target()) {
    SERVER_ERROR("Trying to send ranged-hit message when target is null");
    return;
  }
  userToInform.sendMessage(
      SV_RANGED_NPC_HIT,
      makeArgs(type()->id(), location().x, location().y, target()->location().x,
               target()->location().y));
}

void NPC::sendRangedMissMessageTo(const User &userToInform) const {
  if (!target()) {
    SERVER_ERROR("Trying to send ranged-miss message when target is null");
    return;
  }
  userToInform.sendMessage(
      SV_RANGED_NPC_MISS,
      makeArgs(type()->id(), location().x, location().y, target()->location().x,
               target()->location().y));
}

void NPC::processAI(ms_t timeElapsed) {
  const auto
      AGGRO_RANGE = 70_px,
      ATTACK_RANGE = attackRange() - Podes{1}.toPixels(),
      // Assumption: this is farther than any ranged attack/spell can reach.
      PURSUIT_RANGE = Podes{35}.toPixels(), RETURN_MARGIN = Podes{5}.toPixels();

  target(nullptr);

  // Become aware of nearby users
  for (User *user :
       Server::_instance->findUsersInArea(location(), AGGRO_RANGE)) {
    if (distance(collisionRect(), user->collisionRect()) <= AGGRO_RANGE) {
      makeAwareOf(*user);
    }
  }
  target(_threatTable.getTarget());

  auto distToTarget =
      target() ? distance(collisionRect(), target()->collisionRect()) : 0;

  auto previousState = _state;
  auto previousLocation = location();

  // Transition if necessary
  switch (_state) {
    case IDLE:
      if (combatDamage() == 0 &&
          !npcType()->knownSpell())  // NPCs that can't attack won't try.
        break;

      // React to recent attacker
      if (target()) {
        if (distToTarget > ATTACK_RANGE) {
          _state = CHASE;
          resetLocationUpdateTimer();
        } else
          _state = ATTACK;
      }

      break;

    case CHASE:
      // Target has disappeared
      if (!target()) {
        _state = IDLE;
        break;
      }

      // Target is dead
      if (target()->isDead()) {
        _state = IDLE;
        break;
      }

      // NPC has gone too far from spawn point
      if (spawner()) {
        auto distFromSpawner = spawner()->distanceFromEntity(*this);
        if (distFromSpawner > npcType()->maxDistanceFromSpawner()) {
          _state = IDLE;
          static const auto ATTEMPTS = 20;
          for (auto i = 0; i != ATTEMPTS; ++i) {
            auto dest = spawner()->getRandomPoint();
            if (Server::instance().isLocationValid(dest, *type())) {
              _targetDestination = dest;
              teleportTo(_targetDestination);
              break;
            }
          }
          break;
        }
      }

      // Target has run out of range: give up
      if (distToTarget > PURSUIT_RANGE) {
        _state = IDLE;
        clearTagger();
        break;
      }

      // Target is close enough to attack
      if (distToTarget <= ATTACK_RANGE) {
        _state = ATTACK;
        break;
      }

      break;

    case ATTACK:
      // Target has disappeared
      if (target() == nullptr) {
        _state = IDLE;
        break;
      }

      // Target is dead
      if (target()->isDead()) {
        _state = IDLE;
        break;
      }

      // Target has run out of attack range: chase
      if (distToTarget > ATTACK_RANGE) {
        _state = CHASE;
        resetLocationUpdateTimer();
        break;
      }

      break;
  }

  // On transition
  if ((previousState == CHASE || previousState == ATTACK) && _state == IDLE) {
    target(nullptr);
    _threatTable.clear();
    clearTagger();

    static const auto ATTEMPTS = 20;
    for (auto i = 0; i != ATTEMPTS; ++i) {
      auto dest = spawner()->getRandomPoint();
      if (Server::instance().isLocationValid(dest, *type())) {
        _targetDestination = dest;
        teleportTo(_targetDestination);
        break;
      }
    }

    auto maxHealth = type()->baseStats().maxHealth;
    if (health() < maxHealth) {
      health(maxHealth);
      onHealthChange();  // Only broadcasts to the new location, not the old.

      const Server &server = *Server::_instance;
      for (const User *user : server.findUsersInArea(previousLocation))
        user->sendMessage(SV_ENTITY_HEALTH, makeArgs(serial(), health()));
    }
  }

  // Act
  switch (_state) {
    case IDLE:
      target(nullptr);
      break;

    case CHASE:
      // Move towards player
      updateLocation(target()->location());
      break;

    case ATTACK:
      // Cast any spells it knows
      auto knownSpell = npcType()->knownSpell();
      if (knownSpell) {
        if (isSpellCoolingDown(knownSpell->id())) break;
        ;
        castSpell(*knownSpell);
      }
      break;  // Entity::update() will handle combat
  }
}

void NPC::forgetAbout(const Entity &entity) {
  _threatTable.forgetAbout(entity);
}

void NPC::sendInfoToClient(const User &targetUser) const {
  targetUser.sendMessage(
      SV_OBJECT, makeArgs(serial(), location().x, location().y, type()->id()));

  // Level
  targetUser.sendMessage(SV_NPC_LEVEL, makeArgs(serial(), _level));

  // Hitpoints
  if (health() < stats().maxHealth)
    targetUser.sendMessage(SV_ENTITY_HEALTH, makeArgs(serial(), health()));

  // Loot
  if (!_loot->empty() && tagger() == &targetUser) sendAllLootToTagger();

  // Buffs/debuffs
  for (const auto &buff : buffs())
    targetUser.sendMessage(SV_ENTITY_GOT_BUFF, makeArgs(serial(), buff.type()));
  for (const auto &debuff : debuffs())
    targetUser.sendMessage(SV_ENTITY_GOT_DEBUFF,
                           makeArgs(serial(), debuff.type()));

  // Quests
  QuestNode::sendQuestsToUser(targetUser);
}

ServerItem::Slot *NPC::getSlotToTakeFromAndSendErrors(size_t slotNum,
                                                      const User &user) {
  const Server &server = Server::instance();

  if (_loot->empty()) {
    user.sendMessage(ERROR_EMPTY_SLOT);
    return nullptr;
  }

  if (!server.isEntityInRange(user.socket(), user, this)) return nullptr;

  if (!_loot->isValidSlot(slotNum)) {
    user.sendMessage(ERROR_INVALID_SLOT);
    return nullptr;
  }

  ServerItem::Slot &slot = _loot->at(slotNum);
  if (!slot.first.hasItem()) {
    user.sendMessage(ERROR_EMPTY_SLOT);
    return nullptr;
  }

  return &slot;
}

void NPC::updateStats() {
  const Server &server = *Server::_instance;

  auto oldMaxHealth = stats().maxHealth;
  auto oldMaxEnergy = stats().maxEnergy;

  auto newStats = type()->baseStats();

  // Apply buffs
  for (auto &buff : buffs()) buff.applyStatsTo(newStats);

  // Apply debuffs
  for (auto &debuff : debuffs()) debuff.applyStatsTo(newStats);

  // Assumption: max health/energy won't change

  stats(newStats);
}

void NPC::broadcastDamagedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(), SV_OBJECT_DAMAGED,
                         makeArgs(serial(), amount));
}

void NPC::broadcastHealedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(), SV_OBJECT_HEALED,
                         makeArgs(serial(), amount));
}

int NPC::getLevelDifference(const User &user) const {
  return level() - user.level();
}
