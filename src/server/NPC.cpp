#include "NPC.h"

#include "Server.h"

NPC::NPC(const NPCType *type, const MapPoint &loc)
    : Entity(type, loc),
      QuestNode(*type, serial()),
      _level(type->level()),
      _threatTable(*this),
      _timeSinceLookedForTargets(rand() % AI::FREQUENCY_TO_LOOK_FOR_TARGETS),
      _disappearTimer(type->disappearsAfter()),
      ai(*this) {
  _loot.reset(new Loot);
  onSetType();
}

void NPC::update(ms_t timeElapsed) {
  if (health() > 0 && !isStunned()) ai.process(timeElapsed);

  removeOnTimer(_disappearTimer, timeElapsed);

  Entity::update(timeElapsed);
}

bool NPC::shouldBeIgnoredByAIProximityAggro() const {
  if (npcType()->_aggression == NPCType::Aggression::AGGRESSIVE) return false;
  return true;
}

bool NPC::canBeAttackedBy(const User &user) const {
  if (!npcType()->canBeAttacked()) return false;

  if (permissions.owner().type == Permissions::Owner::MOB) return true;
  if (!permissions.hasOwner()) return false;

  const auto &server = Server::instance();
  auto ownerType = permissions.owner().type == Permissions::Owner::PLAYER
                       ? Belligerent::PLAYER
                       : Belligerent::CITY;
  return server.wars().isAtWar({permissions.owner().name, ownerType},
                               {user.name(), Belligerent::PLAYER});
}

bool NPC::canBeAttackedBy(const NPC &npc) const {
  const auto thisIsUnowned = owner().type == Permissions::Owner::MOB;
  const auto otherIsUnowned = npc.owner().type == Permissions::Owner::MOB;
  if (thisIsUnowned && otherIsUnowned) return false;
  if (thisIsUnowned ^ otherIsUnowned) return true;

  // Both are owned.
  auto thisOwner =
      Belligerent{owner().name, owner().type == Permissions::Owner::PLAYER
                                    ? Belligerent::PLAYER
                                    : Belligerent::CITY};
  auto otherOwner = Belligerent{npc.owner().name,
                                npc.owner().type == Permissions::Owner::PLAYER
                                    ? Belligerent::PLAYER
                                    : Belligerent::CITY};
  const auto &server = Server::instance();
  return server.wars().isAtWar(thisOwner, otherOwner);
}

bool NPC::canBeDamagedAndWarn() const {
  if (ai.state == AI::RETREAT) {
    Server::instance().broadcastToArea(
        location(), {SV_NPC_EVADED_ATTACK, makeArgs(serial())});
    return false;
  }
  return true;
}

bool NPC::canAttack(const Entity &other) const {
  return other.canBeAttackedBy(*this);
}

void NPC::scaleThreatAgainst(Entity &target, double multiplier) {
  _threatTable.scaleThreat(target, multiplier);
}

void NPC::makeAwareOf(Entity &entity) {
  // For when an aggressive NPC begins combat
  if (_threatTable.isEmpty()) _timeEngaged = SDL_GetTicks();

  _threatTable.makeAwareOf(entity);
  makeNearbyNPCsAwareOf(entity);

  auto *user = dynamic_cast<User *>(&entity);
  if (user) user->putInCombat();
}

bool NPC::isAwareOf(Entity &entity) const {
  return _threatTable.isAwareOf(entity);
}

void NPC::makeNearbyNPCsAwareOf(Entity &entity) {
  const Server &server = *Server::_instance;

  const auto CHAIN_PULL_DISTANCE = Podes{10}.toPixels();
  auto nearbyEntities =
      server.findEntitiesInArea(location(), CHAIN_PULL_DISTANCE);
  for (auto nearbyEntity : nearbyEntities) {
    auto npc = dynamic_cast<NPC *>(nearbyEntity);
    if (!npc) continue;

    // Skip chain pulling for neutral NPCs
    if (!npc->npcType()->attacksNearby()) return;

    // Skip those already aware, otherwise we'd get infinite loops
    if (npc->isAwareOf(entity)) continue;

    npc->makeAwareOf(entity);
  }
}

void NPC::addThreat(User &attacker, Threat amount) {
  makeAwareOf(attacker);
  _threatTable.addThreat(attacker, amount);
}

Message NPC::outOfRangeMessage() const {
  return Message(SV_OBJECT_OUT_OF_RANGE, makeArgs(serial()));
}

void NPC::restoreHealthAndBroadcastTo(const MapPoint &p) {
  if (!isMissingHealth()) return;

  health(stats().maxHealth);
  onHealthChange();  // Only broadcasts to the new location, not
                     // the old.

  broadcastHealthTo(p);
}

void NPC::onHealthChange() { broadcastHealthTo(location()); }

void NPC::broadcastHealthTo(const MapPoint &p) {
  const auto healthMsg =
      Message{SV_ENTITY_HEALTH, makeArgs(serial(), health())};
  for (const User *user : Server::_instance->findUsersInArea(p))
    user->sendMessage(healthMsg);
}

void NPC::onDeath() {
  Server &server = *Server::_instance;
  server.forceAllToUntarget(*this);

  do {
    if (owner().type != Permissions::Owner::PLAYER) break;
    if (ai.currentOrder() != AI::ORDER_TO_FOLLOW) break;
    auto *ownerAsOnlineUser = server.getUserByName(owner().name);
    if (!ownerAsOnlineUser) break;
    ownerAsOnlineUser->followers.remove();
  } while (false);

  if (_timeEngaged > 0) {
    auto timeNow = SDL_GetTicks();
    auto timeToKill = timeNow - _timeEngaged;

    auto killerClass = ""s;
    auto killerLevel = 0;
    if (tagger.asUser()) {
      killerClass = tagger.asUser()->getClass().type().id();
      killerLevel = tagger.asUser()->level();
    }

    auto of = std::ofstream{"kills.log", std::ios_base::app};
    of << type()->id()                  // NPC ID
       << "," << level()                // NPC level
       << "," << killerClass            // Killer's class
       << "," << killerLevel            // Killer's level
       << "," << _threatTable.size()    // Entities in threat table
       << "," << timeToKill             // Time between engagement and death
       << "," << npcType()->isRanged()  // Whether NPC is ranged
       << std::endl;

  } else {
    SERVER_ERROR("NPC killed without having been engaged by a user");
  }

  npcType()->lootTable().instantiate(*_loot, tagger.asUser());
  if (!_loot->empty()) sendAllLootToTaggers();

  Entity::onDeath();
}

void NPC::onAttackedBy(Entity &attacker, Threat threat) {
  if (attacker.classTag() == 'u') {
    if (_threatTable.isEmpty()) _timeEngaged = SDL_GetTicks();
    addThreat(dynamic_cast<User &>(attacker), threat);
  }

  makeAwareOf(attacker);

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
      {SV_RANGED_NPC_HIT,
       makeArgs(type()->id(), location().x, location().y,
                target()->location().x, target()->location().y)});
}

void NPC::sendRangedMissMessageTo(const User &userToInform) const {
  if (!target()) {
    SERVER_ERROR("Trying to send ranged-miss message when target is null");
    return;
  }
  userToInform.sendMessage(
      {SV_RANGED_NPC_MISS,
       makeArgs(type()->id(), location().x, location().y,
                target()->location().x, target()->location().y)});
}

void NPC::forgetAbout(const Entity &entity) {
  _threatTable.forgetAbout(entity);
}

double NPC::getTameChance() const {
  return getTameChanceBasedOnHealthPercent(1.0 * health() / stats().maxHealth);
}

void NPC::sendInfoToClient(const User &targetUser, bool isNew) const {
  const Server &server = Server::instance();

  targetUser.sendMessage(
      {SV_OBJECT_INFO,
       makeArgs(serial(), location().x, location().y, type()->id())});

  // Owner
  auto *nonConst = const_cast<NPC *>(this);
  if (nonConst->permissions.hasOwner()) {
    const auto &owner = nonConst->permissions.owner();
    targetUser.sendMessage(
        {SV_OWNER, makeArgs(serial(), owner.typeString(), owner.name)});

    // In case the owner is unknown to the client, tell him the owner's city
    if (owner.type == owner.PLAYER) {
      std::string ownersCity = server.cities().getPlayerCity(owner.name);
      if (!ownersCity.empty())
        targetUser.sendMessage({SV_IN_CITY, makeArgs(owner.name, ownersCity)});
    }
  }

  // Level
  targetUser.sendMessage({SV_NPC_LEVEL, makeArgs(serial(), _level)});

  // Hitpoints
  if (isMissingHealth())
    targetUser.sendMessage({SV_ENTITY_HEALTH, makeArgs(serial(), health())});

  // Loot
  if (!_loot->empty() && tagger == targetUser) sendAllLootToTaggers();

  // Buffs/debuffs
  for (const auto &buff : buffs())
    targetUser.sendMessage(
        {SV_ENTITY_GOT_BUFF, makeArgs(serial(), buff.type())});
  for (const auto &debuff : debuffs())
    targetUser.sendMessage(
        {SV_ENTITY_GOT_DEBUFF, makeArgs(serial(), debuff.type())});

  // Quests
  QuestNode::sendQuestsToUser(targetUser);

  // Transform timer
  if (transformation.isTransforming()) {
    targetUser.sendMessage(
        {SV_TRANSFORM_TIME_REMAINING,
         makeArgs(serial(), transformation.transformTimer())});
  }

  Entity::sendInfoToClient(targetUser, isNew);
}

ServerItem::Instance *NPC::getSlotToTakeFromAndSendErrors(size_t slotNum,
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

  ServerItem::Instance &slot = _loot->at(slotNum);
  if (!slot.hasItem()) {
    user.sendMessage(ERROR_EMPTY_SLOT);
    return nullptr;
  }

  return &slot;
}

void NPC::onOwnershipChange() { target(nullptr); }

void NPC::updateStats() {
  if (isDead()) return;

  const Server &server = *Server::_instance;

  auto oldMaxHealth = stats().maxHealth;
  auto oldMaxEnergy = stats().maxEnergy;

  auto newStats = type()->baseStats();

  // Apply buffs
  for (auto &buff : buffs()) buff.applyStatsTo(newStats);

  // Apply debuffs
  for (auto &debuff : debuffs()) debuff.applyStatsTo(newStats);

  const int healthDecrease = oldMaxHealth - newStats.maxHealth;
  const int oldHealth = health();
  auto newHealth = oldHealth - healthDecrease;
  if (newStats.maxHealth == 0)
    newHealth = 0;
  else if (newHealth < 1)
    newHealth = 1;
  else if (newHealth > static_cast<int>(newStats.maxHealth))
    newHealth = newStats.maxHealth;
  if (healthDecrease != 0) {
    health(newHealth);
    onHealthChange();
  }

  // Assumption: max energy won't change

  stats(newStats);
}

void NPC::broadcastDamagedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(),
                         {SV_OBJECT_DAMAGED, makeArgs(serial(), amount)});
}

void NPC::broadcastHealedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(),
                         {SV_OBJECT_HEALED, makeArgs(serial(), amount)});
}

int NPC::getLevelDifference(const User &user) const {
  return level() - user.level();
}

double NPC::combatDamage() const {
  const auto &stats = npcType()->baseStats();
  const auto bonusMultiplier = npcType()->school() == SpellSchool::PHYSICAL
                                   ? stats.physicalDamage
                                   : stats.magicDamage;
  return bonusMultiplier.addTo(stats.weaponDamage);
}
