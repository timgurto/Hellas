#include <cassert>

#include "ProgressLock.h"
#include "Server.h"
#include "User.h"

ObjectType User::OBJECT_TYPE("__clientObjectType__");

MapPoint User::newPlayerSpawn = {};
double User::spawnRadius = 0;

const std::vector<XP> User::XP_PER_LEVEL{
    // [1] = XP required to get to lvl 2
    // [59] = XP required to get to lvl 60
    0,     600,   1200,  1700,  2300,  2800,  3300,  3800,  4300,  4800,
    5200,  5700,  6100,  6500,  6900,  7300,  7700,  8100,  8400,  8800,
    9100,  9500,  9800,  10100, 10400, 10700, 11000, 11300, 11600, 11900,
    12100, 12400, 12700, 12900, 13200, 13400, 13600, 13900, 14100, 14300,
    14500, 14800, 15000, 15200, 15400, 15600, 15800, 16000, 16100, 16300,
    16500, 16700, 16900, 17000, 17200, 17400, 17500, 17700, 17800, 18000};

User::User(const std::string &name, const MapPoint &loc, const Socket &socket)
    : Object(&OBJECT_TYPE, loc),

      _name(name),
      _socket(socket),

      _action(NO_ACTION),
      _actionTime(0),
      _actionObject(nullptr),
      _actionRecipe(nullptr),
      _actionObjectType(nullptr),
      _actionSlot(INVENTORY_SIZE),
      _actionLocation(0, 0),

      _respawnPoint(newPlayerSpawn),

      _driving(0),

      _inventory(INVENTORY_SIZE),
      _gear(GEAR_SLOTS),
      _lastContact(SDL_GetTicks()) {
  if (!OBJECT_TYPE.collides()) {
    OBJECT_TYPE.collisionRect({-5, -2, 10, 4});
  }
  for (size_t i = 0; i != INVENTORY_SIZE; ++i)
    _inventory[i] = std::make_pair<const ServerItem *, size_t>(0, 0);
}

User::User(const Socket &rhs) : Object(MapPoint{}), _socket(rhs) {}

User::User(const MapPoint &loc) : Object(loc), _socket(Socket::Empty()) {}

void User::init() {
  auto baseStats = Stats{};
  baseStats.armor = 0;
  baseStats.maxHealth = 50;
  baseStats.maxEnergy = 50;
  baseStats.hps = 1;
  baseStats.eps = 1;
  baseStats.hit = 0;
  baseStats.crit = 5;
  baseStats.critResist = 0;
  baseStats.dodge = 5;
  baseStats.block = 5;
  baseStats.blockValue = 0;
  baseStats.magicDamage = 0;
  baseStats.physicalDamage = 0;
  baseStats.healing = 0;
  baseStats.airResist = 0;
  baseStats.earthResist = 0;
  baseStats.fireResist = 0;
  baseStats.waterResist = 0;
  baseStats.attackTime = 1000;
  baseStats.speed = 80.0;
  baseStats.stunned = false;
  baseStats.gatherBonus = 0;
  OBJECT_TYPE.baseStats(baseStats);
}

bool User::compareXThenSerial::operator()(const User *a, const User *b) const {
  if (a->location().x != b->location().x)
    return a->location().x < b->location().x;
  return a->_socket < b->_socket;  // Just need something unique.
}

bool User::compareYThenSerial::operator()(const User *a, const User *b) const {
  if (a->location().y != b->location().y)
    return a->location().y < b->location().y;
  return a->_socket < b->_socket;  // Just need something unique.
}

std::string User::makeLocationCommand() const {
  return makeArgs(_name, location().x, location().y);
}

void User::contact() { _lastContact = SDL_GetTicks(); }

bool User::alive() const {
  return SDL_GetTicks() - _lastContact <= Server::CLIENT_TIMEOUT;
}

size_t User::giveItem(const ServerItem *item, size_t quantity) {
  auto &server = Server::instance();

  auto remaining = quantity;

  // Gear pass 1: partial stacks
  for (auto i = 0; i != GEAR_SLOTS; ++i) {
    if (_gear[i].first != item) continue;
    auto spaceAvailable =
        static_cast<int>(item->stackSize()) - static_cast<int>(_gear[i].second);
    if (spaceAvailable > 0) {
      auto qtyInThisSlot = min(static_cast<size_t>(spaceAvailable), remaining);
      _gear[i].second += qtyInThisSlot;
      Server::instance().sendInventoryMessage(*this, i, Server::GEAR);
      remaining -= qtyInThisSlot;
    }
    if (remaining == 0) break;
  }

  // Inventory pass 1: partial stacks
  if (remaining > 0) {
    for (auto i = 0; i != INVENTORY_SIZE; ++i) {
      if (_inventory[i].first != item) continue;
      assert(remaining > 0);
      assert(item->stackSize() > 0);
      auto spaceAvailable = static_cast<int>(item->stackSize()) -
                            static_cast<int>(_inventory[i].second);
      if (spaceAvailable > 0) {
        auto qtyInThisSlot =
            min(static_cast<size_t>(spaceAvailable), remaining);
        _inventory[i].second += qtyInThisSlot;
        Server::instance().sendInventoryMessage(*this, i, Server::INVENTORY);
        remaining -= qtyInThisSlot;
      }
      if (remaining == 0) break;
    }
  }

  // Inventory pass 2: empty slots
  if (remaining > 0) {
    for (auto i = 0; i != INVENTORY_SIZE; ++i) {
      if (_inventory[i].first != nullptr) continue;
      assert(remaining > 0);
      assert(item->stackSize() > 0);
      auto qtyInThisSlot = min(item->stackSize(), remaining);
      _inventory[i].first = item;
      _inventory[i].second = qtyInThisSlot;
      Server::debug()("Quantity placed in slot: "s + toString(qtyInThisSlot));
      server.sendInventoryMessage(*this, i, Server::INVENTORY);
      remaining -= qtyInThisSlot;
      if (remaining == 0) break;
    }
  }

  // Send client fetch-quest progress
  auto qtyHeld = 0;
  auto qtyHeldHasBeenCalculated = false;
  for (const auto &questID : questsInProgress()) {
    auto quest = server.findQuest(questID);
    if (!quest) continue;

    for (auto i = 0; i != quest->objectives.size(); ++i) {
      const auto &objective = quest->objectives[i];
      if (objective.type != Quest::Objective::FETCH) continue;
      if (objective.id != item->id()) continue;

      // Only count items once
      if (!qtyHeldHasBeenCalculated) {
        qtyHeld = countItems(item);
        qtyHeldHasBeenCalculated = true;
      }

      auto progress = min(qtyHeld, objective.qty);
      server.sendMessage(_socket, SV_QUEST_PROGRESS,
                         makeArgs(questID, i, progress));
    }
  }

  auto quantityGiven = quantity - remaining;
  if (quantityGiven > 0) {
    ProgressLock::triggerUnlocks(*this, ProgressLock::ITEM, item);
    server.sendMessage(_socket, SV_RECEIVED_ITEM,
                       makeArgs(item->id(), quantityGiven));
  }
  return remaining;
}

void User::cancelAction() {
  if (_action == NO_ACTION) return;

  switch (_action) {
    case GATHER:
      _actionObject->decrementGatheringUsers();
  }

  if (_action == ATTACK) {
    resetAttackTimer();
  } else {
    Server::instance().sendMessage(_socket, WARNING_ACTION_INTERRUPTED);
    _action = NO_ACTION;
  }
}

void User::finishAction() {
  if (_action == NO_ACTION) return;

  _action = NO_ACTION;
}

void User::beginGathering(Object *obj) {
  _action = GATHER;
  _actionObject = obj;
  _actionObject->incrementGatheringUsers();
  assert(obj->type());
  _actionTime = obj->objType().gatherTime();
}

void User::beginCrafting(const Recipe &recipe) {
  _action = CRAFT;
  _actionRecipe = &recipe;
  _actionTime = recipe.time();
}

void User::beginConstructing(const ObjectType &obj, const MapPoint &location,
                             size_t slot) {
  _action = CONSTRUCT;
  _actionObjectType = &obj;
  _actionTime = obj.constructionTime();
  _actionSlot = slot;
  _actionLocation = location;
}

void User::beginDeconstructing(Object &obj) {
  _action = DECONSTRUCT;
  _actionObject = &obj;
  _actionTime = obj.deconstruction().timeToDeconstruct();
}

void User::setTargetAndAttack(Entity *target) {
  this->target(target);
  if (target == nullptr) {
    cancelAction();
    return;
  }
  _action = ATTACK;
}

bool User::hasItems(const ItemSet &items) const {
  ItemSet remaining = items;
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    const std::pair<const ServerItem *, size_t> &invSlot = _inventory[i];
    remaining.remove(invSlot.first, invSlot.second);
    if (remaining.isEmpty()) return true;
  }
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const std::pair<const ServerItem *, size_t> &gearSlot = _gear[i];
    remaining.remove(gearSlot.first, gearSlot.second);
    if (remaining.isEmpty()) return true;
  }
  return false;
}

bool User::hasItems(const std::string &tag, size_t quantity) const {
  auto remaining = quantity;
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    const std::pair<const ServerItem *, size_t> &invSlot = _inventory[i];
    if (!invSlot.first) continue;
    if (invSlot.first->isTag(tag)) {
      if (invSlot.second >= remaining) return true;
      remaining -= invSlot.second;
    }
  }
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const std::pair<const ServerItem *, size_t> &gearSlot = _gear[i];
    if (!gearSlot.first) continue;
    if (gearSlot.first->isTag(tag)) {
      if (gearSlot.second >= remaining) return true;
      remaining -= gearSlot.second;
    }
  }
  return false;
}

bool User::hasTool(const std::string &tagName) const {
  // Check gear
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const ServerItem *item = _gear[i].first;
    if (item && item->isTag(tagName)) return true;
  }

  // Check inventory
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    const ServerItem *item = _inventory[i].first;
    if (item && item->isTag(tagName)) return true;
  }

  // Check nearby terrain
  Server &server = *Server::_instance;
  auto nearbyTerrain =
      server.nearbyTerrainTypes(collisionRect(), Server::ACTION_DISTANCE);
  for (char terrainType : nearbyTerrain) {
    if (server.terrainType(terrainType)->tag() == tagName) return true;
  }

  // Check nearby objects
  // Note that checking collision chunks means ignoring non-colliding objects.
  auto superChunk = Server::_instance->getCollisionSuperChunk(location());
  for (CollisionChunk *chunk : superChunk)
    for (const auto &pair : chunk->entities()) {
      const Entity *pEnt = pair.second;
      const Object *pObj = dynamic_cast<const Object *>(pEnt);
      if (pObj == nullptr) continue;
      if (pObj->isBeingBuilt()) continue;
      if (!pObj->type()->isTag(tagName)) continue;
      if (distance(pObj->collisionRect(), collisionRect()) >
          Server::ACTION_DISTANCE)
        continue;
      if (!pObj->permissions().doesUserHaveAccess(_name)) continue;

      return true;
    }

  return false;
}

bool User::hasTools(const std::set<std::string> &classes) const {
  for (const std::string &tagName : classes)
    if (!hasTool(tagName)) return false;
  return true;
}

static void removeItemsFrom(ItemSet &remaining, ServerItem::vect_t &container,
                            std::set<size_t> &slotsChanged) {
  slotsChanged = {};
  for (size_t i = 0; i != container.size(); ++i) {
    auto &slot = container[i];
    auto &itemType = slot.first;
    auto &qty = slot.second;
    if (remaining.contains(itemType)) {
      size_t itemsToRemove = min(qty, remaining[itemType]);
      remaining.remove(itemType, itemsToRemove);
      qty -= itemsToRemove;
      if (qty == 0) itemType = nullptr;
      slotsChanged.insert(i);
      if (remaining.isEmpty()) break;
    }
  }
}

void User::removeItems(const ItemSet &items) {
  auto remaining = items;
  std::set<size_t> slotsChanged;

  removeItemsFrom(remaining, _inventory, slotsChanged);
  for (size_t slotNum : slotsChanged)
    Server::instance().sendInventoryMessage(*this, slotNum, Server::INVENTORY);

  removeItemsFrom(remaining, _gear, slotsChanged);
  for (size_t slotNum : slotsChanged)
    Server::instance().sendInventoryMessage(*this, slotNum, Server::GEAR);

  assert(remaining.isEmpty());
}

static void removeItemsFrom(const std::string &tag, size_t &remaining,
                            ServerItem::vect_t &container,
                            std::set<size_t> &slotsChanged) {
  if (remaining == 0) return;

  slotsChanged = {};
  for (size_t i = 0; i != container.size(); ++i) {
    auto &slot = container[i];
    auto &itemType = slot.first;
    auto &qty = slot.second;
    if (itemType == nullptr) continue;
    if (itemType->isTag(tag)) {
      size_t itemsToRemove = min(qty, remaining);
      remaining -= itemsToRemove;

      qty -= itemsToRemove;
      if (qty == 0) itemType = nullptr;

      slotsChanged.insert(i);
      if (remaining == 0) break;
    }
  }
}

void User::removeItems(const std::string &tag, size_t quantity) {
  std::set<size_t> slotsChanged;
  auto remaining = quantity;

  removeItemsFrom(tag, remaining, _inventory, slotsChanged);
  for (size_t slotNum : slotsChanged)
    Server::instance().sendInventoryMessage(*this, slotNum, Server::INVENTORY);

  removeItemsFrom(tag, remaining, _gear, slotsChanged);
  for (size_t slotNum : slotsChanged)
    Server::instance().sendInventoryMessage(*this, slotNum, Server::GEAR);
}

int User::countItems(const ServerItem *item) const {
  auto count = 0;

  for (auto &pair : _gear)
    if (pair.first == item) count += pair.second;

  for (auto &pair : _inventory)
    if (pair.first == item) count += pair.second;

  return count;
}

void User::update(ms_t timeElapsed) {
  regen(timeElapsed);

  if (_action == NO_ACTION) {
    Entity::update(timeElapsed);
    return;
  }

  Server &server = *Server::_instance;

  if (isStunned()) {
    cancelAction();
    return;
  }

  if (_actionTime > timeElapsed)
    _actionTime -= timeElapsed;
  else
    _actionTime = 0;

  if (_actionTime > 0) {  // Action hasn't finished yet.
    Entity::update(timeElapsed);
    return;
  }

  // Timer has finished; complete action
  switch (_action) {
    case ATTACK:
      break;  // All handled by Entity::update()

    case GATHER:
      if (!_actionObject->contents().isEmpty())
        server.gatherObject(_actionObject->serial(), *this);
      break;

    case CRAFT: {
      if (!hasRoomToCraft(*_actionRecipe)) {
        server.sendMessage(_socket, WARNING_INVENTORY_FULL);
        cancelAction();
        return;
      }
      // Remove materials from user's inventory
      removeItems(_actionRecipe->materials());
      // Give user his newly crafted items
      const ServerItem *product = toServerItem(_actionRecipe->product());
      giveItem(product, _actionRecipe->quantity());
      // Trigger any new unlocks
      ProgressLock::triggerUnlocks(*this, ProgressLock::RECIPE, _actionRecipe);
      break;
    }

    case CONSTRUCT: {
      // Create object
      server.addObject(_actionObjectType, _actionLocation, _name);
      if (_actionSlot ==
          INVENTORY_SIZE)  // Constructing an object without an item
        break;
      // Remove item from user's inventory
      std::pair<const ServerItem *, size_t> &slot = _inventory[_actionSlot];
      assert(slot.first->constructsObject() == _actionObjectType);
      --slot.second;
      if (slot.second == 0) slot.first = nullptr;
      server.sendInventoryMessage(*this, _actionSlot, Server::INVENTORY);
      // Trigger any new unlocks
      ProgressLock::triggerUnlocks(*this, ProgressLock::CONSTRUCTION,
                                   _actionObjectType);
      break;
    }

    case DECONSTRUCT: {
      // Check for inventory space
      const ServerItem *item = _actionObject->deconstruction().becomes();
      if (!vectHasSpace(_inventory, item)) {
        server.sendMessage(_socket, WARNING_INVENTORY_FULL);
        cancelAction();
        return;
      }
      // Give user his item
      giveItem(item);
      // Remove object
      server.removeEntity(*_actionObject);
      break;
    }

    default:
      assert(false);
  }

  if (_action != ATTACK) {  // ATTACK is a repeating action.
    server.sendMessage(_socket, SV_ACTION_FINISHED);
    finishAction();
  }

  Entity::update(timeElapsed);
}

bool User::hasRoomToCraft(const Recipe &recipe) const {
  size_t slotsFreedByMaterials = 0;
  ItemSet remainingMaterials = recipe.materials();
  ServerItem::vect_t inventoryCopy = _inventory;
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    std::pair<const ServerItem *, size_t> &invSlot = inventoryCopy[i];
    if (remainingMaterials.contains(invSlot.first)) {
      size_t itemsToRemove =
          min(invSlot.second, remainingMaterials[invSlot.first]);
      remainingMaterials.remove(invSlot.first, itemsToRemove);
      inventoryCopy[i].second -= itemsToRemove;
      if (inventoryCopy[i].second == 0) inventoryCopy[i].first = nullptr;
      if (remainingMaterials.isEmpty()) break;
    }
  }
  return vectHasSpace(inventoryCopy, toServerItem(recipe.product()),
                      recipe.quantity());
}

bool User::shouldGatherDoubleThisTime() const {
  return randDouble() * 100 <= stats().gatherBonus;
}

const MapRect User::collisionRect() const {
  return OBJECT_TYPE.collisionRect() + location();
}

bool User::canBeAttackedBy(const User &user) const {
  const Server &server = *Server::_instance;
  return server._wars.isAtWar({_name}, {user._name});
}

px_t User::attackRange() const {
  const auto weapon = _gear[Item::WEAPON_SLOT].first;
  if (weapon == nullptr) return Object::attackRange();
  return weapon->weaponRange();
}

CombatResult User::generateHitAgainst(const Entity &target, CombatType type,
                                      SpellSchool school, px_t range) const {
  const auto BASE_MISS_CHANCE = Percentage{10};

  auto levelDiff = target.level() - level();

  auto roll = rand() % 100;

  // Miss
  auto missChance = BASE_MISS_CHANCE - stats().hit + levelDiff;
  missChance = max(0, missChance);
  if (combatTypeCanHaveOutcome(type, MISS, school, range)) {
    if (roll < missChance) return MISS;
    roll -= missChance;
  }

  // Dodge
  auto dodgeChance = target.stats().dodge + levelDiff;
  dodgeChance = max(0, dodgeChance);
  if (combatTypeCanHaveOutcome(type, DODGE, school, range)) {
    if (roll < dodgeChance) return DODGE;
    roll -= dodgeChance;
  }

  // Block
  auto blockChance = target.stats().block + levelDiff;
  blockChance = max(0, blockChance);
  if (target.canBlock() &&
      combatTypeCanHaveOutcome(type, BLOCK, school, range)) {
    if (roll < blockChance) return BLOCK;
    roll -= blockChance;
  }

  // Crit
  auto critChance = stats().crit - target.stats().critResist - levelDiff;
  critChance = max(0, critChance);
  if (critChance > 0 && combatTypeCanHaveOutcome(type, CRIT, school, range)) {
    if (roll < critChance) return CRIT;
    roll -= critChance;
  }

  return HIT;
}

void User::sendGotHitMessageTo(const User &user) const {
  Server::_instance->sendMessage(user.socket(), SV_PLAYER_WAS_HIT, _name);
}

bool User::canBlock() const {
  auto offhandItem = _gear[Item::OFFHAND_SLOT].first;
  if (offhandItem == nullptr) return false;
  return offhandItem->isTag("shield");
}

SpellSchool User::school() const {
  auto weapon = _gear[Item::WEAPON_SLOT].first;
  if (!weapon) return {};
  return weapon->weaponSchool();
}

double User::combatDamage() const {
  const auto BASE_DAMAGE = 5.0;

  auto weapon = _gear[Item::WEAPON_SLOT].first;
  auto damageSchool = weapon ? weapon->weaponSchool() : SpellSchool::PHYSICAL;

  if (damageSchool == SpellSchool::PHYSICAL)
    return BASE_DAMAGE + stats().physicalDamage;
  else
    return BASE_DAMAGE + stats().magicDamage;
}

void User::onHealthChange() {
  const Server &server = *Server::_instance;
  for (const User *userToInform : server.findUsersInArea(location()))
    server.sendMessage(userToInform->socket(), SV_PLAYER_HEALTH,
                       makeArgs(_name, health()));
  Object::onHealthChange();
}

void User::onEnergyChange() {
  const Server &server = *Server::_instance;
  for (const User *userToInform : server.findUsersInArea(location()))
    server.sendMessage(userToInform->socket(), SV_PLAYER_ENERGY,
                       makeArgs(_name, energy()));
  Object::onEnergyChange();
}

void User::onDeath() {
  Server &server = *Server::_instance;
  server.forceAllToUntarget(*this);

  // Handle respawn etc.
  moveToSpawnPoint();

  auto talentLost = _class.value().loseARandomLeafTalent();
  if (!talentLost.empty()) {
    const Server &server = *Server::_instance;
    server.sendMessage(_socket, SV_LOST_TALENT, talentLost);
  }

  health(stats().maxHealth);
  energy(stats().maxEnergy);
  onHealthChange();
}

void User::onNewOwnedObject(const ObjectType &type) const {
  if (type.isPlayerUnique())
    this->_playerUniqueCategoriesOwned.insert(type.playerUniqueCategory());
}

void User::onDestroyedOwnedObject(const ObjectType &type) const {
  if (!type.isPlayerUnique()) return;
  this->_playerUniqueCategoriesOwned.erase(type.playerUniqueCategory());
}

void User::onKilled(Entity &victim) {
  auto levelDiff = victim.getLevelDifference(*this);

  auto xp = XP{};
  if (levelDiff < -9)
    xp = 0;
  else if (levelDiff > 5)
    xp = 150;
  else {
    auto xpPerLevelDiff = std::unordered_map<int, XP>{
        {-9, 13}, {-8, 26}, {-7, 38}, {-6, 49}, {-5, 59},
        {-4, 68}, {-3, 77}, {-2, 85}, {-1, 93}, {0, 100},
        {1, 107}, {2, 115}, {3, 123}, {4, 132}, {5, 141}};
    xp = xpPerLevelDiff[levelDiff];
  }

  addXP(xp);

  auto victimID = victim.type()->id();
  addQuestProgress(Quest::Objective::KILL, victimID);
}

void User::addQuestProgress(Quest::Objective::Type type,
                            const std::string &id) {
  const auto &server = Server::instance();
  for (const auto &questID : _quests) {
    auto quest = server.findQuest(questID);
    for (auto i = 0; i != quest->objectives.size(); ++i) {
      auto &objective = quest->objectives[i];
      if (objective.type != type) continue;
      if (objective.id != id) continue;

      auto key = QuestProgress{};
      key.ID = objective.id;
      key.type = objective.type;
      key.quest = questID;

      auto &progress = _questProgress[key];
      if (progress == objective.qty) continue;

      progress = min(progress + 1, objective.qty);

      // Alert user
      server.sendMessage(_socket, SV_QUEST_PROGRESS,
                         makeArgs(questID, i, progress));
      if (quest->canBeCompletedByUser(*this))
        server.sendMessage(_socket, SV_QUEST_CAN_BE_FINISHED, questID);

      break;  // Assuming there will be a key match no more than once per quest
    }
  }
}

void User::initQuestProgress(const Quest::ID &questID,
                             Quest::Objective::Type type, const std::string &id,
                             int qty) {
  auto key = QuestProgress{};
  key.quest = questID;
  key.ID = id;
  key.type = type;

  _questProgress[key] = qty;
}

int User::questProgress(const Quest::ID &quest, Quest::Objective::Type type,
                        const std::string &id) const {
  auto key = QuestProgress{};
  key.quest = quest;
  key.ID = id;
  key.type = type;

  auto it = _questProgress.find(key);
  if (it == _questProgress.end()) return 0;
  return it->second;
}

bool User::canAttack() const {
  const auto &gearSlot = _gear[Item::WEAPON_SLOT];

  auto hasWeapon = gearSlot.first != nullptr;
  if (!hasWeapon) return true;

  auto weapon = gearSlot.first;
  if (!weapon->usesAmmo()) return true;

  auto ammoType = gearSlot.first->weaponAmmo();
  auto itemSet = ItemSet{};
  itemSet.add(ammoType, 1);
  if (this->hasItems(itemSet)) return true;

  auto ammoID = gearSlot.first->weaponAmmo()->id();
  Server::_instance->sendMessage(_socket, WARNING_OUT_OF_AMMO, ammoID);
  return false;
}

void User::onAttack() {
  // Tag target
  if (!target()->tagger()) target()->tagger(*this);

  // Remove ammo if ranged weapon
  auto weapon = _gear[Item::WEAPON_SLOT].first;
  if (!weapon) return;
  auto ammoType = weapon->weaponAmmo();
  if (!ammoType) return;
  auto ammo = ItemSet{};
  ammo.add(ammoType);
  removeItems(ammo);
}

void User::sendRangedHitMessageTo(const User &userToInform) const {
  if (!target()) return;
  auto weapon = _gear[Item::WEAPON_SLOT].first;
  if (!weapon) return;

  Server &server = *Server::_instance;
  server.sendMessage(userToInform.socket(), SV_RANGED_WEAPON_HIT,
                     makeArgs(weapon->id(), location().x, location().y,
                              target()->location().x, target()->location().y));
}

void User::sendRangedMissMessageTo(const User &userToInform) const {
  if (!target()) return;
  auto weapon = _gear[Item::WEAPON_SLOT].first;
  if (!weapon) return;

  Server &server = *Server::_instance;
  server.sendMessage(userToInform.socket(), SV_RANGED_WEAPON_MISS,
                     makeArgs(weapon->id(), location().x, location().y,
                              target()->location().x, target()->location().y));
}

void User::broadcastDamagedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(), SV_PLAYER_DAMAGED,
                         makeArgs(_name, amount));
}

void User::broadcastHealedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(), SV_PLAYER_HEALED, makeArgs(_name, amount));
}

void User::updateStats() {
  const Server &server = *Server::_instance;

  auto oldMaxHealth = stats().maxHealth;
  auto oldMaxEnergy = stats().maxEnergy;

  auto newStats = OBJECT_TYPE.baseStats();

  // Apply talents
  getClass().applyStatsTo(newStats);

  // Apply gear
  for (size_t i = 0; i != GEAR_SLOTS; ++i) {
    const ServerItem *item = _gear[i].first;
    if (item != nullptr) newStats &= item->stats();
  }

  // Apply buffs
  for (auto &buff : buffs()) buff.applyStatsTo(newStats);

  // Apply debuffs
  for (auto &debuff : debuffs()) debuff.applyStatsTo(newStats);

  // Special case: health must change to reflect new max health
  int healthDecrease = oldMaxHealth - newStats.maxHealth;
  if (healthDecrease != 0) {
    // Alert nearby users to new max health
    server.broadcastToArea(location(), SV_MAX_HEALTH,
                           makeArgs(_name, newStats.maxHealth));
  }
  int oldHealth = health();
  auto newHealth = oldHealth - healthDecrease;
  if (newHealth < 1)  // Implicit rule: changing gear can never kill you, only
                      // reduce you to 1 health.
    newHealth = 1;
  else if (newHealth > static_cast<int>(newStats.maxHealth))
    newHealth = newStats.maxHealth;
  if (healthDecrease != 0) {
    health(newHealth);
    onHealthChange();
  }

  int energyDecrease = oldMaxEnergy - newStats.maxEnergy;
  if (energyDecrease != 0) {
    // Alert nearby users to new max energy
    server.broadcastToArea(location(), SV_MAX_ENERGY,
                           makeArgs(_name, newStats.maxEnergy));
  }
  int oldEnergy = energy();
  auto newEnergy = oldEnergy - energyDecrease;
  if (newEnergy < 1)  // Implicit rule: changing gear can never kill you, only
                      // reduce you to 1 health.
    newEnergy = 1;
  else if (newEnergy > static_cast<int>(newStats.maxEnergy))
    newEnergy = newStats.maxEnergy;
  if (energyDecrease != 0) {
    health(newEnergy);
    onEnergyChange();
  }

  auto args = makeArgs(
      makeArgs(newStats.armor, newStats.maxHealth, newStats.maxEnergy,
               newStats.hps, newStats.eps),
      makeArgs(newStats.hit, newStats.crit, newStats.critResist, newStats.dodge,
               newStats.block, newStats.blockValue),
      makeArgs(newStats.magicDamage, newStats.physicalDamage, newStats.healing),
      makeArgs(newStats.airResist, newStats.earthResist, newStats.fireResist,
               newStats.waterResist),
      makeArgs(newStats.attackTime, newStats.speed));
  server.sendMessage(socket(), SV_YOUR_STATS, args);

  stats(newStats);
}

bool User::knowsConstruction(const std::string &id) const {
  const Server &server = *Server::_instance;
  const ObjectType *objectType = server.findObjectTypeByName(id);
  bool objectTypeExists = (objectType != nullptr);
  if (!objectTypeExists) return false;
  if (objectType->isKnownByDefault()) return true;
  bool userKnowsConstruction =
      _knownConstructions.find(id) != _knownConstructions.end();
  return userKnowsConstruction;
}

bool User::knowsRecipe(const std::string &id) const {
  const Server &server = *Server::_instance;
  auto it = server._recipes.find(id);
  bool recipeExists = (it != server._recipes.end());
  if (!recipeExists) return false;
  if (it->isKnownByDefault()) return true;
  bool userKnowsRecipe = _knownRecipes.find(id) != _knownRecipes.end();
  return userKnowsRecipe;
}

void User::sendInfoToClient(const User &targetUser) const {
  const Server &server = Server::instance();
  const Socket &client = targetUser.socket();

  bool isSelf = &targetUser == this;

  // Location
  server.sendMessage(client, SV_LOCATION, makeLocationCommand());

  // Hitpoints
  server.sendMessage(client, SV_MAX_HEALTH, makeArgs(_name, stats().maxHealth));
  server.sendMessage(client, SV_PLAYER_HEALTH, makeArgs(_name, health()));

  // Energy
  server.sendMessage(client, SV_MAX_ENERGY, makeArgs(_name, stats().maxEnergy));
  server.sendMessage(client, SV_PLAYER_ENERGY, makeArgs(_name, energy()));

  // Class
  server.sendMessage(client, SV_CLASS,
                     makeArgs(_name, getClass().type().id(), _level));

  // XP
  if (isSelf) sendXPMessage();

  // City
  const City::Name city = server._cities.getPlayerCity(_name);
  if (!city.empty())
    server.sendMessage(client, SV_IN_CITY, makeArgs(_name, city));

  // King?
  if (server._kings.isPlayerAKing(_name))
    server.sendMessage(client, SV_KING, _name);

  // Gear
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const ServerItem *item = gear(i).first;
    if (item != nullptr)
      server.sendMessage(client, SV_GEAR, makeArgs(_name, i, item->id()));
  }

  // Buffs/debuffs
  for (const auto &buff : buffs())
    server.sendMessage(client, SV_PLAYER_GOT_BUFF,
                       makeArgs(_name, buff.type()));
  for (const auto &debuff : debuffs())
    server.sendMessage(client, SV_PLAYER_GOT_DEBUFF,
                       makeArgs(_name, debuff.type()));

  // Vehicle?
  if (isDriving())
    server.sendMessage(client, SV_MOUNTED, makeArgs(driving(), _name));
}

void User::onOutOfRange(const Entity &rhs) const {
  if (rhs.shouldAlwaysBeKnownToUser(*this)) return;

  const Server &server = *Server::_instance;
  auto message = rhs.outOfRangeMessage();
  server.sendMessage(socket(), message.code, message.args);
}

Message User::outOfRangeMessage() const {
  return Message(SV_USER_OUT_OF_RANGE, name());
}

void User::moveToSpawnPoint(bool isNewPlayer) {
  Server &server = Server::instance();

  MapPoint newLoc;
  size_t attempts = 0;
  static const size_t MAX_ATTEMPTS = 1000;
  do {
    if (attempts > MAX_ATTEMPTS) {
      server._debug("Failed to find valid spawn location for user",
                    Color::FAILURE);
      return;
    }
    server._debug << "Attempt #" << ++attempts << " at placing new user"
                  << Log::endl;
    newLoc.x = (randDouble() * 2 - 1) * spawnRadius + _respawnPoint.x;
    newLoc.y = (randDouble() * 2 - 1) * spawnRadius + _respawnPoint.y;
  } while (!server.isLocationValid(newLoc, User::OBJECT_TYPE));
  auto oldLoc = location();
  location(newLoc, /* firstInsertion */ isNewPlayer);

  if (isNewPlayer) return;

  server.broadcastToArea(oldLoc, SV_LOCATION_INSTANT,
                         makeArgs(name(), location().x, location().y));
  server.broadcastToArea(location(), SV_LOCATION_INSTANT,
                         makeArgs(name(), location().x, location().y));

  server.sendRelevantEntitiesToUser(*this);
}

void User::startQuest(const Quest &quest) {
  _quests.insert(quest.id);

  auto message =
      quest.hasObjective() ? SV_QUEST_IN_PROGRESS : SV_QUEST_CAN_BE_FINISHED;
  auto &server = Server::instance();
  server.sendMessage(_socket, message, quest.id);

  for (const auto &itemID : quest.startsWithItems) {
    auto item = server.findItem(itemID);
    if (!item) return;
    giveItem(item);
  }
}

void User::completeQuest(const Quest::ID &id) {
  auto &server = Server::instance();
  const auto quest = server.findQuest(id);
  for (const auto &objective : quest->objectives) {
    if (objective.type == Quest::Objective::FETCH) {
      auto set = ItemSet{};
      set.add(objective.item, objective.qty);
      this->removeItems(set);
    }
  }

  markQuestAsCompleted(id);
  _quests.erase(id);

  addXP(100);

  if (quest->otherQuestHasThisAsPrerequisite())
    server.sendMessage(_socket, SV_QUEST_CAN_BE_STARTED,
                       quest->otherQuestWithThisAsPrerequisite);

  server.sendMessage(_socket, SV_QUEST_COMPLETED, id);
}

bool User::hasCompletedQuest(const Quest::ID &id) const {
  auto it = _questsCompleted.find(id);
  return it != _questsCompleted.end();
}

void User::markQuestAsCompleted(const Quest::ID &id) {
  _questsCompleted.insert(id);
}

void User::markQuestAsStarted(const Quest::ID &id) { _quests.insert(id); }

void User::sendBuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(), SV_PLAYER_GOT_BUFF, makeArgs(_name, buff));
}

void User::sendDebuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(), SV_PLAYER_GOT_DEBUFF,
                         makeArgs(_name, buff));
}

void User::sendLostBuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(), SV_PLAYER_LOST_BUFF,
                         makeArgs(_name, buff));
}

void User::sendLostDebuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(), SV_PLAYER_LOST_DEBUFF,
                         makeArgs(_name, buff));
}

void User::sendXPMessage() const {
  const Server &server = Server::instance();
  server.sendMessage(_socket, SV_XP, makeArgs(_xp, XP_PER_LEVEL[_level]));
}

void User::announceLevelUp() const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(), SV_LEVEL_UP, _name);
}

void User::addXP(XP amount) {
  if (_level == MAX_LEVEL) return;
  _xp += amount;

  Server &server = Server::instance();
  server.sendMessage(_socket, SV_XP_GAIN, makeArgs(amount));
  sendXPMessage();

  const auto maxXpThisLevel = XP_PER_LEVEL[_level];
  if (_xp < maxXpThisLevel) return;

  levelUp();

  if (_level == MAX_LEVEL)
    _xp = 0;
  else {
    auto surplus = _xp - maxXpThisLevel;
    _xp = surplus;
  }
}

void User::levelUp() {
  ++_level;
  fillHealthAndEnergy();
  announceLevelUp();
}

bool User::QuestProgress::operator<(const QuestProgress &rhs) const {
  if (quest != rhs.quest) return quest < rhs.quest;
  if (type != rhs.type) return type < rhs.type;
  return ID < rhs.ID;
}
