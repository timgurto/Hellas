#include "User.h"

#include <thread>

#include "../curlUtil.h"
#include "../threadNaming.h"
#include "Groups.h"
#include "ProgressLock.h"
#include "Server.h"

ObjectType User::OBJECT_TYPE("__clientObjectType__");

MapPoint User::newPlayerSpawn = {};
MapPoint User::postTutorialSpawn = {};
double User::spawnRadius = 0;

const std::vector<XP> User::XP_PER_LEVEL{
    // [1] = XP required to get to lvl 2
    // [59] = XP required to get to lvl 60
    0,     600,   1200,  1700,  2300,  2800,  3300,  3800,  4300,  4800,  5200,
    5700,  6100,  6500,  6900,  7300,  7700,  8100,  8400,  8800,  9100,  9500,
    9800,  10100, 10400, 10700, 11000, 11300, 11600, 11900, 12100, 12400, 12700,
    12900, 13200, 13400, 13600, 13900, 14100, 14300, 14500, 14800, 15000, 15200,
    15400, 15600, 15800, 16000, 16100, 16300, 16500, 16700, 16900, 17000, 17200,
    17400, 17500, 17700, 17800, 18000, 99999};

#define RETURN_WITH(MSG) \
  {                      \
    sendMessage(MSG);    \
    return;              \
  }

User::User(const std::string &name, const MapPoint &loc, const Socket *socket)
    : Object(&OBJECT_TYPE, loc),

      _name(name),

      _action(NO_ACTION),
      _actionTime(0),
      _actionObject(nullptr),
      _actionRecipe(nullptr),
      _actionObjectType(nullptr),
      _actionSlot(INVENTORY_SIZE),
      _actionLocation(0, 0),

      exploration(Server::instance().map().width(),
                  Server::instance().map().height()),

      _respawnPoint(newPlayerSpawn),

      _inventory(INVENTORY_SIZE),
      _gear(GEAR_SLOTS),
      _lastContact(SDL_GetTicks()) {
  if (socket) _socket = *socket;

  // Once-off init
  if (!OBJECT_TYPE.collides()) {
    OBJECT_TYPE.collisionRect({-5, -2, 10, 4});
  }
}

User::User(const Socket &rhs)
    : Object(MapPoint{}), _socket(rhs), exploration(0, 0) {}

User::User(const MapPoint &loc)
    : Object(loc), _socket(Socket::Empty()), exploration(0, 0) {}

void User::initialiseInventoryAndGear() {
  for (size_t i = 0; i != INVENTORY_SIZE; ++i)
    _inventory[i] = {
        nullptr, ServerItem::Instance::ReportingInfo::UserInventory(this, i),
        0};
  for (size_t i = 0; i != GEAR_SLOTS; ++i)
    _gear[i] = {nullptr, ServerItem::Instance::ReportingInfo::UserGear(this, i),
                0};
}

void User::findRealWorldLocationStatic() {
  auto &server = Server::instance();
  server.incrementThreadCount();

  auto result = getLocationFromIP(socket().ip());

  if (!Server::hasInstance()) return;
  auto pUser = server.getUserByName(name());
  if (pUser) pUser->_realWorldLocation = result;

  server.decrementThreadCount();
}

void User::findRealWorldLocation() {
  std::thread([this]() {
    setThreadName("Finding user location");
    findRealWorldLocationStatic();
  }).detach();
}

const std::string &User::realWorldLocation() const {
  static const auto default = "{}"s;
  return _realWorldLocation.empty() ? default : _realWorldLocation;
}

int User::secondsPlayedThisSession() const {
  auto ticksThisSession = SDL_GetTicks() - _serverTicksAtLogin;
  return toInt(ticksThisSession / 1000.0);
}

int User::secondsPlayed() const {
  return _secondsPlayedBeforeThisSession + secondsPlayedThisSession();
}

void User::sendTimePlayed() const {
  sendMessage({SV_TIME_PLAYED, makeArgs(secondsPlayed())});
}

Message User::teleportMessage(const MapPoint &destination) const {
  return {SV_USER_LOCATION_INSTANT,
          makeArgs(name(), destination.x, destination.y)};
}

void User::onTeleport() {
  Server::instance().sendRelevantEntitiesToUser(*this);
}

bool User::hasRoomFor(std::set<std::string> itemNames) const {
  auto &s = Server::instance();

  // Work with copies
  auto inventory = _inventory;
  auto gear = _gear;

  for (const auto &itemName : itemNames) {
    auto remaining = 1;  // Assumption: one of each item type
    auto itemAdded = false;

    const auto *item = s.findItem(itemName);
    if (!item) continue;
    if (item->stackSize() == 0) {
      SERVER_ERROR("Item with stack size 0");
      return false;
    }

    // Gear pass 1: partial stacks
    for (auto i = 0; i != GEAR_SLOTS; ++i) {
      if (gear[i].type() != item) continue;
      auto spaceAvailable = static_cast<int>(item->stackSize()) -
                            static_cast<int>(gear[i].quantity());
      if (spaceAvailable > 0) {
        gear[i].addItems(1);
        itemAdded = true;
        break;
      }
    }
    if (itemAdded) continue;  // Next item

    // Inventory pass 1: partial stacks
    for (auto i = 0; i != INVENTORY_SIZE; ++i) {
      if (inventory[i].type() != item) continue;

      if (itemAdded) {
        SERVER_ERROR(
            "Trying to find room for an item that has already been added");
        return false;
      }

      auto spaceAvailable = static_cast<int>(item->stackSize()) -
                            static_cast<int>(inventory[i].quantity());
      if (spaceAvailable > 0) {
        inventory[i].addItems(1);
        itemAdded = true;
        break;
      }
    }
    if (itemAdded) continue;  // Next item

    // Inventory pass 2: empty slots
    for (auto i = 0; i != INVENTORY_SIZE; ++i) {
      auto slotIsEmpty = !inventory[i].hasItem();
      if (!slotIsEmpty) continue;

      if (itemAdded) {
        SERVER_ERROR(
            "Trying to find room for an item that has already been added");
        return false;
      }

      inventory[i] = {item, ServerItem::Instance::ReportingInfo::DummyUser(),
                      1};
      itemAdded = true;
      break;
    }

    if (!itemAdded) return false;
  }
  return true;
}

void User::init() {
  auto baseStats = Stats{};
  baseStats.armor = 0;
  baseStats.maxHealth = 50;
  baseStats.maxEnergy = 50;
  baseStats.hps = 100;
  baseStats.eps = 100;
  baseStats.hit = 0;
  baseStats.crit = 500;
  baseStats.critResist = 0;
  baseStats.dodge = 500;
  baseStats.block = 500;
  baseStats.blockValue = 0;
  baseStats.magicDamage = 0;
  baseStats.physicalDamage = 0;
  baseStats.healing = 0;
  baseStats.airResist = 0;
  baseStats.earthResist = 0;
  baseStats.fireResist = 0;
  baseStats.waterResist = 0;
  baseStats.weaponDamage = 2;
  baseStats.attackTime = 2000;
  baseStats.speed = 80.0;
  baseStats.stunned = false;
  baseStats.gatherBonus = 0;
  baseStats.followerLimit = 1;
  OBJECT_TYPE.baseStats(baseStats);
}

bool User::compareXThenSocketThenAddress::operator()(const User *a,
                                                     const User *b) const {
  if (a->location().x != b->location().x)
    return a->location().x < b->location().x;
  if (a->_socket.value() != b->_socket.value())
    return a->_socket.value() < b->_socket.value();
  return a < b;
}

bool User::compareYThenSocketThenAddress::operator()(const User *a,
                                                     const User *b) const {
  if (a->location().y != b->location().y)
    return a->location().y < b->location().y;
  if (a->_socket.value() != b->_socket.value())
    return a->_socket.value() < b->_socket.value();
  return a < b;
}

std::string User::makeLocationCommand() const {
  return makeArgs(_name, location().x, location().y);
}

void User::contact() { _lastContact = SDL_GetTicks(); }

bool User::hasExceededTimeout() const {
  const auto timeAllowed = isInitialised()
                               ? Server::CLIENT_TIMEOUT_AFTER_LOGIN
                               : Server::CLIENT_TIMEOUT_BEFORE_LOGIN;
  return SDL_GetTicks() - _lastContact > timeAllowed;
}

size_t User::giveItem(const ServerItem *item, size_t quantity, Hitpoints health,
                      std::string suffix) {
  auto &server = Server::instance();
  auto suffixChosen = ""s;

  auto remaining = quantity;

  if (item->stackSize() > 1) {
    // Assumption: stacking items can't be damaged (thus this pass ignores
    // health) Gear pass 1: partial stacks
    for (auto i = 0; i != GEAR_SLOTS; ++i) {
      if (_gear[i].type() != item) continue;
      auto spaceAvailable = static_cast<int>(item->stackSize()) -
                            static_cast<int>(_gear[i].quantity());
      if (spaceAvailable > 0) {
        auto qtyInThisSlot =
            min(static_cast<size_t>(spaceAvailable), remaining);
        _gear[i].addItems(qtyInThisSlot);
        Server::instance().sendInventoryMessage(*this, i, Serial::Gear());
        remaining -= qtyInThisSlot;
      }
      if (remaining == 0) break;
    }

    // Inventory pass 1: partial stacks
    if (remaining > 0) {
      for (auto i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].type() != item) continue;

        if (remaining == 0) {
          SERVER_ERROR(
              "Trying to find room for an item that has already been added");
          return false;
        }

        if (item->stackSize() == 0) {
          SERVER_ERROR("Item with stack size 0");
          return false;
        }

        auto spaceAvailable = static_cast<int>(item->stackSize()) -
                              static_cast<int>(_inventory[i].quantity());
        if (spaceAvailable > 0) {
          auto qtyInThisSlot =
              min(static_cast<size_t>(spaceAvailable), remaining);
          _inventory[i].addItems(qtyInThisSlot);
          Server::instance().sendInventoryMessage(*this, i,
                                                  Serial::Inventory());
          remaining -= qtyInThisSlot;
        }
        if (remaining == 0) break;
      }
    }
  }

  // Inventory pass 2: empty slots
  if (remaining > 0) {
    for (auto i = 0; i != INVENTORY_SIZE; ++i) {
      if (_inventory[i].hasItem()) continue;

      if (remaining == 0) {
        SERVER_ERROR(
            "Trying to find room for an item that has already been added");
        return false;
      }

      if (item->stackSize() == 0) {
        SERVER_ERROR("Item with stack size 0");
        return false;
      }

      auto qtyInThisSlot = min(item->stackSize(), remaining);
      _inventory[i] = ServerItem::Instance{
          item, ServerItem::Instance::ReportingInfo::UserInventory(this, i),
          qtyInThisSlot};
      _inventory[i].initHealth(health);
      if (!suffix.empty()) _inventory[i].setSuffix(suffix);
      suffixChosen = _inventory[i].suffix();
      server.sendInventoryMessage(*this, i, Serial::Inventory());
      remaining -= qtyInThisSlot;
      if (remaining == 0) break;
    }
  }

  // Send client fetch-quest progress
  auto qtyHeld = 0;
  auto qtyHeldHasBeenCalculated = false;
  for (const auto &pair : questsInProgress()) {
    const auto &questID = pair.first;
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
      sendMessage({SV_QUEST_PROGRESS, makeArgs(questID, i, progress)});
      if (quest->canBeCompletedByUser(*this))
        sendMessage({SV_QUEST_CAN_BE_FINISHED, questID});
    }
  }

  auto quantityGiven = quantity - remaining;
  if (quantityGiven > 0) {
    ProgressLock::triggerUnlocks(*this, ProgressLock::ITEM, item);
    sendMessage(
        {SV_RECEIVED_ITEM, makeArgs(item->id(), suffixChosen, quantityGiven)});
  }
  return remaining;
}

void User::cancelAction() {
  if (_action == NO_ACTION) return;

  switch (_action) {
    case GATHER:
      _actionObject->gatherable.decrementGatheringUsers();
      break;

    case CRAFT:
      Server::instance().broadcastToArea(location(),
                                         {SV_PLAYER_STOPPED_CRAFTING, _name});
      break;
  }

  if (_action == ATTACK) {
    // resetAttackTimer();
    _shouldSuppressAmmoWarnings = false;
  } else {
    sendMessage(WARNING_ACTION_INTERRUPTED);
    _action = NO_ACTION;
  }
}

void User::finishAction() {
  if (_action == NO_ACTION) return;

  if (_action == CRAFT)
    Server::instance().broadcastToArea(location(),
                                       {SV_PLAYER_STOPPED_CRAFTING, _name});

  _action = NO_ACTION;
}

void User::beginGathering(Entity *ent, double speedMultiplier) {
  _action = GATHER;
  _actionObject = ent;
  _actionObject->gatherable.incrementGatheringUsers();
  if (!ent->type()) {
    SERVER_ERROR("Can't gather from object with no type");
    return;
  }
  _actionTime = toInt(ent->type()->yield.gatherTime() / speedMultiplier);

  sendMessage({SV_ACTION_STARTED, _actionTime});
}

void User::beginCrafting(const SRecipe &recipe, double speed) {
  _action = CRAFT;
  _actionRecipe = &recipe;
  _actionTime = toInt(recipe.time() / speed);

  sendMessage({SV_ACTION_STARTED, _actionTime});
  Server::instance().broadcastToArea(
      location(), {SV_PLAYER_STARTED_CRAFTING, makeArgs(_name, recipe.id())});
}

void User::beginConstructing(const ObjectType &obj, const MapPoint &location,
                             bool cityOwned, double speedMultiplier,
                             size_t slot) {
  _action = CONSTRUCT;
  _actionObjectType = &obj;
  _actionTime = toInt(obj.constructionTime() / speedMultiplier);
  _actionSlot = slot;
  _actionLocation = location;
  _actionOwnedByCity = cityOwned;

  sendMessage({SV_ACTION_STARTED, _actionTime});
}

void User::beginDeconstructing(Object &obj) {
  _action = DECONSTRUCT;
  _actionObject = &obj;
  _actionTime = obj.deconstruction().timeToDeconstruct();
}

void User::setTargetAndAttack(Entity *target) {
  _shouldSuppressAmmoWarnings = false;
  this->target(target);
  if (target == nullptr) {
    cancelAction();
    return;
  }
  _action = ATTACK;
}

void User::alertReactivelyTargetingUser(const User &targetingUser) const {
  targetingUser.sendMessage({SV_YOU_ARE_ATTACKING_PLAYER, _name});
}

void User::tryToConstruct(const std::string &id, const MapPoint &location,
                          Permissions::Owner::Type owner) {
  auto &server = Server::instance();
  auto *objType = server.findObjectTypeByID(id);
  if (!objType) RETURN_WITH(ERROR_INVALID_OBJECT)

  if (!knowsConstruction(id)) RETURN_WITH(ERROR_UNKNOWN_CONSTRUCTION)
  if (objType->isUnbuildable()) RETURN_WITH(ERROR_UNBUILDABLE)

  // Must be last due to RETURN_WITH macros inside.
  tryToConstructInner(*objType, location, owner);
}

void User::tryToConstructFromItem(size_t slot, const MapPoint &location,
                                  Permissions::Owner::Type owner) {
  if (slot >= INVENTORY_SIZE) RETURN_WITH(ERROR_INVALID_SLOT)
  const auto &invSlot = inventory(slot);
  if (!invSlot.hasItem()) RETURN_WITH(ERROR_EMPTY_SLOT)
  const ServerItem &item = *invSlot.type();
  if (!item.constructsObject()) RETURN_WITH(ERROR_CANNOT_CONSTRUCT);
  const ObjectType &objType = *item.constructsObject();

  if (invSlot.isBroken()) RETURN_WITH(WARNING_BROKEN_ITEM)

  // Must be last due to RETURN_WITH macros inside.
  tryToConstructInner(objType, location, owner, slot);
}

void User::tryToConstructInner(const ObjectType &objType,
                               const MapPoint &location,
                               Permissions::Owner::Type owner, size_t slot) {
  auto &server = Server::instance();

  if (isStunned()) RETURN_WITH(WARNING_STUNNED)

  if (objType.isUnique() && objType.numInWorld() == 1)
    RETURN_WITH(WARNING_UNIQUE_OBJECT)
  if (objType.isPlayerUnique() &&
      hasPlayerUnique(objType.playerUniqueCategory())) {
    sendMessage({WARNING_PLAYER_UNIQUE_OBJECT, objType.playerUniqueCategory()});
    return;
  }

  if (distance(collisionRect(), objType.collisionRect() + location) >
      Server::ACTION_DISTANCE)
    RETURN_WITH(WARNING_TOO_FAR)
  if (!server.isLocationValid(location, objType)) RETURN_WITH(WARNING_BLOCKED)

  // Tool check must be the last check, as it damages the tools.
  auto requiresTool = !objType.constructionReq().empty();
  auto toolSpeed = 1.0;
  if (requiresTool) {
    toolSpeed = checkAndDamageToolAndGetSpeed(objType.constructionReq());
    if (toolSpeed == 0) RETURN_WITH(WARNING_NEED_TOOLS)
  }

  cancelAction();
  removeInterruptibleBuffs();

  const auto ownerIsCity = owner == Permissions::Owner::CITY;

  beginConstructing(objType, location, ownerIsCity, toolSpeed, slot);
  sendMessage({SV_ACTION_STARTED, objType.constructionTime() / toolSpeed});
}

bool User::hasItems(const ItemSet &items) const {
  ItemSet remaining = items;
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    const auto &invSlot = _inventory[i];
    remaining.remove(invSlot.type(), invSlot.quantity());
    if (remaining.isEmpty()) return true;
  }
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const auto &gearSlot = _gear[i];
    remaining.remove(gearSlot.type(), gearSlot.quantity());
    if (remaining.isEmpty()) return true;
  }
  return false;
}

bool User::hasItems(const std::string &tag, size_t quantity) const {
  auto remaining = quantity;
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    const auto &invSlot = _inventory[i];
    if (!invSlot.hasItem()) continue;
    if (invSlot.type()->isTag(tag)) {
      if (invSlot.quantity() >= remaining) return true;
      remaining -= invSlot.quantity();
    }
  }
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const auto &gearSlot = _gear[i];
    if (!gearSlot.hasItem()) continue;
    if (gearSlot.type()->isTag(tag)) {
      if (gearSlot.quantity() >= remaining) return true;
      remaining -= gearSlot.quantity();
    }
  }
  return false;
}

User::ToolSearchResult User::findTool(const std::string &tagName) {
  auto bestSpeed = 0.0;
  auto bestTool = User::ToolSearchResult{User::ToolSearchResult::NOT_FOUND};

  // Check gear
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    auto &slot = _gear[i];
    const auto *type = slot.type();
    if (!slot.hasItem()) continue;
    if (!type->isTag(tagName)) continue;

    auto toolSpeed = type->toolSpeed(tagName);
    if (toolSpeed > bestSpeed || !bestTool) {
      bestSpeed = toolSpeed;
      bestTool = {slot, *type, tagName};
    }
  }

  // Check inventory
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    auto &slot = _inventory[i];
    const auto *type = slot.type();
    if (!slot.hasItem()) continue;
    if (!type->isTag(tagName)) continue;

    auto toolSpeed = type->toolSpeed(tagName);
    if (toolSpeed > bestSpeed || !bestTool) {
      bestSpeed = toolSpeed;
      bestTool = {slot, *type, tagName};
    }
  }

  // Check nearby terrain
  Server &server = *Server::_instance;
  auto nearbyTerrain = server.map().terrainTypesOverlapping(
      collisionRect(), Server::ACTION_DISTANCE);
  for (char terrainType : nearbyTerrain) {
    const auto *terrain = server.terrainType(terrainType);
    if (!terrain->isTag(tagName)) continue;

    auto toolSpeed = terrain->toolSpeed(tagName);
    if (toolSpeed > bestSpeed || !bestTool) {
      bestSpeed = toolSpeed;
      bestTool = {ToolSearchResult::TERRAIN, *terrain, tagName};
    }
  }

  // Check nearby objects
  // Note that checking collision chunks means ignoring non-colliding objects.
  auto nearbyEntities = server.findEntitiesInArea(location());
  for (auto *pEnt : nearbyEntities) {
    auto *pObj = dynamic_cast<Object *>(pEnt);
    if (!pObj) continue;
    if (pObj->isBeingBuilt()) continue;
    const auto *type = pObj->type();
    if (!type->isTag(tagName)) continue;
    if (distance(*pObj, *this) > Server::ACTION_DISTANCE) continue;
    if (!pObj->permissions.canUserUseAsTool(_name)) continue;

    auto toolSpeed = type->toolSpeed(tagName);
    if (toolSpeed > bestSpeed || !bestTool) {
      bestSpeed = toolSpeed;
      bestTool = {*pObj, *type, tagName};
    }
  }

  return bestTool;
}

double User::checkAndDamageToolsAndGetSpeed(const std::set<std::string> &tags) {
  auto toolsFound = std::vector<ToolSearchResult>{};
  for (const std::string &tagName : tags) {
    auto result = findTool(tagName);
    if (!result) return 0;
    toolsFound.push_back(result);
  }

  auto speed = 1.0;
  for (const auto &tool : toolsFound) speed *= tool.toolSpeed();

  // At this point, all tools were found and true will be returned.  Only now
  // should all tools be damaged.
  for (const auto &tool : toolsFound) tool.use();

  return speed;
}

double User::checkAndDamageToolAndGetSpeed(const std::string &tag) {
  auto tool = findTool(tag);
  if (!tool) return 0;
  tool.use();
  return tool.toolSpeed();
}

bool User::hasRoomForMoreFollowers() const {
  return followers.num() < stats().followerLimit;
}

void User::clearInventory() {
  const Server &server = *Server::_instance;
  for (auto i = 0; i != INVENTORY_SIZE; ++i)
    if (_inventory[i].hasItem()) {
      _inventory[i] = {};
      server.sendMessage(socket(), {SV_INVENTORY, makeArgs(Serial::Inventory(),
                                                           i, ""s, 0, 0, ""s)});
    }
}

void User::clearGear() {
  const Server &server = *Server::_instance;
  for (auto i = 0; i != GEAR_SLOTS; ++i)
    if (_gear[i].hasItem()) {
      _gear[i] = {};
      server.sendMessage(socket(), {SV_INVENTORY, makeArgs(Serial::Gear(), i,
                                                           ""s, 0, 0, ""s)});
    }
}

static std::multimap<size_t, size_t> containerSlotsByQuantity(
    const ServerItem::vect_t &container) {
  auto slotsOrderedByQuantity = std::multimap<size_t, size_t>{};
  for (auto i = 0; i != container.size(); ++i)
    slotsOrderedByQuantity.insert({container[i].quantity(), i});
  return slotsOrderedByQuantity;
}

static void removeItemsFromContainer(ItemSet &remaining,
                                     ServerItem::vect_t &container,
                                     std::set<size_t> &slotsChanged) {
  slotsChanged = {};

  for (auto pair : containerSlotsByQuantity(container)) {
    auto i = pair.second;
    auto &slot = container[i];
    if (remaining.contains(slot.type())) {
      size_t itemsToRemove = min(slot.quantity(), remaining[slot.type()]);
      remaining.remove(slot.type(), itemsToRemove);
      slot.removeItems(itemsToRemove);
      if (slot.quantity() == 0) slot = {};
      slotsChanged.insert(i);
      if (remaining.isEmpty()) break;
    }
  }
}

ItemSet User::removeItems(const ItemSet &items) {
  auto remaining = items;
  std::set<size_t> slotsChanged;

  removeItemsFromContainer(remaining, _inventory, slotsChanged);
  for (size_t slotNum : slotsChanged)
    Server::instance().sendInventoryMessage(*this, slotNum,
                                            Serial::Inventory());

  removeItemsFromContainer(remaining, _gear, slotsChanged);
  for (size_t slotNum : slotsChanged)
    Server::instance().sendInventoryMessage(*this, slotNum, Serial::Gear());

  return remaining;
}

static void removeItemsMatchingTagFromContainer(
    const std::string &tag, size_t &remaining, ServerItem::vect_t &container,
    std::set<size_t> &slotsChanged) {
  if (remaining == 0) return;

  slotsChanged = {};
  for (size_t i = 0; i != container.size(); ++i) {
    auto &slot = container[i];
    if (!slot.hasItem()) continue;
    if (slot.type()->isTag(tag)) {
      size_t itemsToRemove = min(slot.quantity(), remaining);
      remaining -= itemsToRemove;

      slot.removeItems(itemsToRemove);
      if (slot.quantity() == 0) slot = {};

      slotsChanged.insert(i);
      if (remaining == 0) break;
    }
  }
}

void User::removeItemsMatchingTag(const std::string &tag, size_t quantity) {
  std::set<size_t> slotsChanged;
  auto remaining = quantity;

  removeItemsMatchingTagFromContainer(tag, remaining, _inventory, slotsChanged);
  for (size_t slotNum : slotsChanged)
    Server::instance().sendInventoryMessage(*this, slotNum,
                                            Serial::Inventory());

  removeItemsMatchingTagFromContainer(tag, remaining, _gear, slotsChanged);
  for (size_t slotNum : slotsChanged)
    Server::instance().sendInventoryMessage(*this, slotNum, Serial::Gear());
}

int User::countItems(const ServerItem *item) const {
  auto count = 0;

  for (auto &slot : _gear)
    if (slot.type() == item) count += slot.quantity();

  for (auto &slot : _inventory)
    if (slot.type() == item) count += slot.quantity();

  return count;
}

void User::sendMessage(const Message &msg) const {
  if (!_socket.hasValue()) return;
  const Server &server = Server::instance();
  server.sendMessage(_socket.value(), msg);
}

void User::update(ms_t timeElapsed) {
  // Quests
  auto questsToAbandon = std::set<std::string>{};
  for (auto &pair : _quests) {
    // No time limit
    if (pair.second == 0) continue;

    // Time has run out
    if (timeElapsed >= pair.second) {
      questsToAbandon.insert(pair.first);
      sendMessage({SV_QUEST_FAILED, pair.first});
      continue;
    }

    // Count down
    pair.second -= timeElapsed;
  }
  for (auto questID : questsToAbandon) abandonQuest(questID);

  if (_action == NO_ACTION) {
    Entity::update(timeElapsed);
    return;
  }

  Server &server = *Server::_instance;

  if (isStunned()) {
    cancelAction();
    Entity::update(timeElapsed);
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
      if (_actionObject->gatherable.hasItems())
        server.gatherObject(_actionObject->serial(), *this);
      break;

    case CRAFT: {
      if (!hasRoomToCraft(*_actionRecipe)) {
        sendMessage(WARNING_INVENTORY_FULL);
        cancelAction();
        Entity::update(timeElapsed);
        return;
      }

      removeItems(_actionRecipe->materials());

      const auto *product = toServerItem(_actionRecipe->product());
      giveItem(product, _actionRecipe->quantity());

      if (_actionRecipe->byproduct()) {
        const auto *byproduct = toServerItem(_actionRecipe->byproduct());
        giveItem(byproduct, _actionRecipe->byproductQty());
      }

      ProgressLock::triggerUnlocks(*this, ProgressLock::RECIPE, _actionRecipe);
      break;
    }

    case CONSTRUCT: {
      auto owner = Permissions::Owner{};
      if (_actionOwnedByCity && server.cities().isPlayerInACity(_name)) {
        owner.type = Permissions::Owner::CITY;
        owner.name = server.cities().getPlayerCity(_name);
      } else {
        owner.type = Permissions::Owner::PLAYER;
        owner.name = _name;
      }

      // Create object
      server.addObject(_actionObjectType, _actionLocation, owner);

      addQuestProgress(Quest::Objective::CONSTRUCT, _actionObjectType->id());

      const auto wasBuiltFromItem = _actionSlot != INVENTORY_SIZE;
      if (!wasBuiltFromItem) break;

      // Remove item from user's inventory
      auto &slot = _inventory[_actionSlot];
      if (!slot.hasItem() ||
          slot.type()->constructsObject() != _actionObjectType) {
        SERVER_ERROR("Trying to construct object from an invalid item");
        break;
      }
      slot.removeItems(1);
      if (slot.quantity() == 0) slot = {};
      server.sendInventoryMessage(*this, _actionSlot, Serial::Inventory());

      // Trigger any new unlocks
      ProgressLock::triggerUnlocks(*this, ProgressLock::CONSTRUCTION,
                                   _actionObjectType);
      break;
    }

    case DECONSTRUCT: {
      auto *obj = dynamic_cast<Object *>(_actionObject);
      if (!obj) break;

      // Check for inventory space
      const ServerItem *item = obj->deconstruction().becomes();
      if (!vectHasSpace(_inventory, item)) {
        sendMessage(WARNING_INVENTORY_FULL);
        cancelAction();
        break;
      }
      // Give user his item
      giveItem(item);
      // Remove object
      server.removeEntity(*obj);
      break;
    }

    default:
      SERVER_ERROR("Unhandled message");
  }

  if (_action != ATTACK) {  // ATTACK is a repeating action.
    sendMessage(SV_ACTION_FINISHED);
    finishAction();
  }

  Entity::update(timeElapsed);
}

bool User::hasRoomToRemoveThenAdd(ItemSet toBeRemoved,
                                  ItemSet toBeAdded) const {
  ServerItem::vect_t inventoryCopy = _inventory;
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    auto &invSlot = inventoryCopy[i];
    if (toBeRemoved.contains(invSlot.type())) {
      size_t itemsToRemove =
          min(invSlot.quantity(), toBeRemoved[invSlot.type()]);
      toBeRemoved.remove(invSlot.type(), itemsToRemove);
      inventoryCopy[i].removeItems(itemsToRemove);
      if (inventoryCopy[i].quantity() == 0) inventoryCopy[i] = {};
      if (toBeRemoved.isEmpty()) break;
    }
  }
  return vectHasSpace(inventoryCopy, toBeAdded);
}

bool User::hasRoomToCraft(const SRecipe &recipe) const {
  auto productsAndByproducts = ItemSet{};
  productsAndByproducts.add(recipe.product(), recipe.quantity());
  productsAndByproducts.add(recipe.byproduct(), recipe.byproductQty());
  return hasRoomToRemoveThenAdd(recipe.materials(), productsAndByproducts);
}

bool User::shouldGatherDoubleThisTime() const {
  return randDouble() * 100 <= stats().gatherBonus;
}

void User::setHotbarAction(size_t button, int category, const std::string &id) {
  if (button >= _hotbar.size()) {
    auto slotsToAdd = button - _hotbar.size() + 1;
    for (auto i = 0; i != slotsToAdd; ++i) {
      _hotbar.push_back({});
    }
  }

  _hotbar[button].category = HotbarCategory(category);
  _hotbar[button].id = id;
}

void User::sendHotbarMessage() {
  auto args = makeArgs(_hotbar.size());
  for (auto &slot : _hotbar) {
    args = makeArgs(args, slot.category, slot.id);
  }
  sendMessage({SV_HOTBAR, args});
}

void User::markTutorialAsCompleted() {
  _isInTutorial = false;
  sendMessage({SV_YOU_HAVE_FINISHED_THE_TUTORIAL});
}

void User::onMove() {
  auto &server = Server::instance();

  // Explore map
  auto chunk = exploration.getChunk(location());
  auto newlyExploredChunks = exploration.explore(chunk);
  for (const auto &chunk : newlyExploredChunks)
    exploration.sendSingleChunk(socket(), chunk);
  if (!newlyExploredChunks.empty()) {
    auto of = std::ofstream{"exploration.log", std::ios_base::app};
    of << _name                                   // Player name
       << "," << exploration.numChunksExplored()  // Chunks explored
       << "," << secondsPlayed()                  // Time played (s)
       << std::endl;
  }

  // Get buffs from objects
  auto buffsToAdd = std::map<const BuffType *, Entity *>{};
  for (auto *entity : server.findEntitiesInArea(location())) {
    const Object *pObj = dynamic_cast<const Object *>(entity);
    if (pObj == nullptr) continue;
    if (!pObj->permissions.canUserGetBuffs(_name)) continue;
    if (pObj->isBeingBuilt()) continue;
    const auto &objType = pObj->objType();
    if (!objType.grantsBuff()) continue;
    if (distance(*pObj, *this) > objType.buffRadius()) continue;

    buffsToAdd[objType.buffGranted()] = entity;
  }

  // Remove any disqualified pre-existing object buffs
  auto buffsToRemove = std::set<std::string>{};
  for (const auto &currentlyActiveBuff : buffs()) {
    const auto *buffType = server.findBuff(currentlyActiveBuff.type());

    if (!buffType) {
      SERVER_ERROR("Trying to remove nonexistent buff '"s +
                   currentlyActiveBuff.type() + "'"s);
      continue;
    }

    if (!buffType->grantedByObject()) continue;

    if (buffsToAdd.count(buffType) == 0) buffsToRemove.insert(buffType->id());
  }

  for (const auto buffID : buffsToRemove) removeBuff(buffID);

  // Add buffs from objects
  for (const auto &pair : buffsToAdd) {
    applyBuff(*pair.first, *pair.second);
  }
}

const MapRect User::collisionRect() const {
  return OBJECT_TYPE.collisionRect() + location();
}

bool User::collides() const {
  if (isDriving()) return false;
  return Object::collides();
}

bool User::canBeAttackedBy(const User &user) const {
  const Server &server = *Server::_instance;
  return server._wars.isAtWar({_name}, {user._name});
}

bool User::canBeAttackedBy(const NPC &npc) const {
  if (npc.owner().type == Permissions::Owner::MOB) return true;
  auto ownerBelligerentType = npc.owner().type == Permissions::Owner::PLAYER
                                  ? Belligerent::PLAYER
                                  : Belligerent::CITY;
  return Server::instance().wars().isAtWar(
      {npc.owner().name, ownerBelligerentType}, {_name, Belligerent::PLAYER});
}

bool User::canAttack(const Entity &other) const {
  return other.canBeAttackedBy(*this);
}

px_t User::attackRange() const {
  const auto &weapon = _gear[Item::WEAPON];
  if (!weapon.hasItem()) return Object::attackRange();
  return weapon.type()->weaponRange();
}

void User::sendGotHitMessageTo(const User &user) const {
  Server::_instance->sendMessage(user.socket(), {SV_PLAYER_WAS_HIT, _name});
}

bool User::canBlock() const {
  const auto &offhandItem = _gear[Item::OFFHAND];
  if (!offhandItem.hasItem()) return false;
  if (!offhandItem.type()->isTag("shield")) return false;
  if (offhandItem.isBroken()) return false;
  return true;
}

SpellSchool User::school() const {
  const auto &weapon = _gear[Item::WEAPON];
  if (!weapon.hasItem()) return {};
  return weapon.type()->stats().weaponSchool;
}

bool User::canEquip(const ServerItem &item) const {
  return _level >= item.lvlReq();
}

double User::combatDamage() const {
  if (stats().weaponSchool == SpellSchool::PHYSICAL)
    return stats().physicalDamage.addTo(stats().weaponDamage);
  else
    return stats().magicDamage.addTo(stats().weaponDamage);
}

void User::onHealthChange() {
  const Server &server = *Server::_instance;
  for (const User *userToInform : server.findUsersInArea(location()))
    server.sendMessage(userToInform->socket(),
                       {SV_PLAYER_HEALTH, makeArgs(_name, health())});
  Object::onHealthChange();
}

void User::onEnergyChange() {
  const Server &server = *Server::_instance;
  for (const User *userToInform : server.findUsersInArea(location()))
    server.sendMessage(userToInform->socket(),
                       {SV_PLAYER_ENERGY, makeArgs(_name, energy())});
  Object::onEnergyChange();
}

void User::onDeath() {
  health(stats().maxHealth);
  energy(stats().maxEnergy);
  onHealthChange();
  onEnergyChange();

  Server &server = *Server::_instance;

  server.broadcastToArea(location(), SV_A_PLAYER_DIED);

  server.forceAllToUntarget(*this);
  setTargetAndAttack(nullptr);
  cancelAction();
  _action = NO_ACTION;

  isWaitingForDeathAcknowledgement = true;
  sendMessage(SV_YOU_DIED);

  removeAllBuffsAndDebuffs();

  for (auto &slot : _gear) {
    if (!slot.hasItem()) continue;
    if (!slot.type()->canBeDamaged()) continue;
    slot.damageOnPlayerDeath();
  }

  // Handle respawn etc.
  moveToSpawnPoint();

  auto talentLost = _class.value().loseARandomLeafTalent();
  if (!talentLost.empty()) {
    const Server &server = *Server::_instance;
    sendMessage({SV_LOST_TALENT, talentLost});
  }
}

void User::accountForOwnedEntities() const {
  auto &server = Server::instance();
  auto its = server.findObjectsOwnedBy({Permissions::Owner::PLAYER, _name});
  for (auto it = its.first; it != its.second; ++it) {
    auto serial = *it;
    auto *ent = server.findEntityBySerial(serial);
    if (!ent) continue;
    if (ent->isDead()) continue;
    ent->accountForOwnershipByUser(*this);
  }
}

void User::registerObjectIfPlayerUnique(const ObjectType &type) const {
  if (type.isPlayerUnique())
    this->_playerUniqueCategoriesOwned.insert(type.playerUniqueCategory());
}

void User::deregisterDestroyedObjectIfPlayerUnique(
    const ObjectType &type) const {
  if (!type.isPlayerUnique()) return;
  this->_playerUniqueCategoriesOwned.erase(type.playerUniqueCategory());
}

void User::onAttackedBy(Entity &attacker, Threat threat) {
  cancelAction();

  auto armourSlotToUse = Item::getRandomArmorSlot();
  _gear[armourSlotToUse].onUseInCombat();

  // Fight back if no current target
  if (!target() && attacker.canBeAttackedBy(*this)) {
    setTargetAndAttack(&attacker);
    attacker.alertReactivelyTargetingUser(*this);
  }

  // Sick nearby pets on the attacker
  for (auto *pet : findNearbyPets()) pet->makeAwareOf(attacker);

  Object::onAttackedBy(attacker, threat);
}

void User::onKilled(Entity &victim) {
  if (victim.grantsXPOnDeath()) {
    auto xp = appropriateXPForKill(victim);
    addXP(xp);
  }

  auto victimID = victim.type()->id();
  addQuestProgress(Quest::Objective::KILL, victimID);
}

void User::addQuestProgress(Quest::Objective::Type type,
                            const std::string &id) {
  const auto &server = Server::instance();
  for (const auto &pair : _quests) {
    const auto &questID = pair.first;
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
      sendMessage({SV_QUEST_PROGRESS, makeArgs(questID, i, progress)});
      if (quest->canBeCompletedByUser(*this))
        sendMessage({SV_QUEST_CAN_BE_FINISHED, questID});

      break;  // Assuming there will be a key match no more than once per
              // quest
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

bool User::canStartQuest(const Quest::ID &id) const {
  Server &server = *Server::_instance;

  if (hasCompletedQuest(id)) return false;

  if (isOnQuest(id)) return false;

  if (!hasCompletedAllPrerequisiteQuestsOf(id)) return false;

  auto *quest = server.findQuest(id);
  if (!quest->exclusiveToClass.empty() &&
      quest->exclusiveToClass != getClass().type().id())
    return false;

  return true;
}

bool User::canAttack() {
  const auto &gearSlot = _gear[Item::WEAPON];

  auto hasWeapon = gearSlot.hasItem();
  if (!hasWeapon) return true;

  auto *weapon = gearSlot.type();
  if (!weapon->usesAmmo()) return true;

  auto ammoType = weapon->weaponAmmo();
  auto itemSet = ItemSet{};
  itemSet.add(ammoType, 1);
  if (this->hasItems(itemSet)) return true;

  auto ammoID = weapon->weaponAmmo()->id();
  if (!_shouldSuppressAmmoWarnings) {
    sendMessage({WARNING_OUT_OF_AMMO, ammoID});
    _shouldSuppressAmmoWarnings = true;
  }
  return false;
}

void User::onCanAttack() { _shouldSuppressAmmoWarnings = false; }

void User::onAttack() {
  // Remove ammo if ranged weapon
  auto &weapon = _gear[Item::WEAPON];
  if (!weapon.hasItem()) return;

  auto usesAmmo = weapon.type()->weaponAmmo() != nullptr;
  if (usesAmmo) {
    auto ammo = ItemSet{};
    ammo.add(weapon.type()->weaponAmmo());
    removeItems(ammo);

    // If the weapon itself was used as ammo and there are none left
    if (!weapon.hasItem()) updateStats();
  }

  // Damage the weapon
  auto weaponIsConsumedByAttack =
      usesAmmo && weapon.type() && weapon.type() == weapon.type()->weaponAmmo();
  if (weaponIsConsumedByAttack) return;
  weapon.onUseInCombat();
  if (weapon.isBroken()) updateStats();
}

void User::onSuccessfulSpellcast(const std::string &id, const Spell &spell) {
  Entity::onSuccessfulSpellcast(id, spell);

  if (spell.cooldown() != 0)
    sendMessage({SV_SPELL_COOLING_DOWN, makeArgs(id, spell.cooldown())});

  addQuestProgress(Quest::Objective::CAST_SPELL, id);
}

void User::sendRangedHitMessageTo(const User &userToInform) const {
  if (!target()) return;
  const auto *weapon = _gear[Item::WEAPON].type();
  if (!weapon) return;

  Server &server = *Server::_instance;
  server.sendMessage(
      userToInform.socket(),
      {SV_RANGED_WEAPON_HIT,
       makeArgs(weapon->id(), location().x, location().y,
                target()->location().x, target()->location().y)});
}

void User::sendRangedMissMessageTo(const User &userToInform) const {
  if (!target()) return;
  const auto *weapon = _gear[Item::WEAPON].type();
  if (!weapon) return;

  Server &server = *Server::_instance;
  server.sendMessage(
      userToInform.socket(),
      {SV_RANGED_WEAPON_MISS,
       makeArgs(weapon->id(), location().x, location().y,
                target()->location().x, target()->location().y)});
}

void User::broadcastDamagedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(),
                         {SV_PLAYER_DAMAGED, makeArgs(_name, amount)});
}

void User::broadcastHealedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(),
                         {SV_PLAYER_HEALED, makeArgs(_name, amount)});
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
    const auto &item = _gear[i];
    if (!item.hasItem()) continue;
    if (item.isBroken()) continue;
    if (!canEquip(*item.type())) continue;

    newStats &= item.type()->stats();
    newStats &= item.statsFromSuffix();
  }

  // Apply buffs
  for (auto &buff : buffs()) buff.applyStatsTo(newStats);

  // Apply debuffs
  for (auto &debuff : debuffs()) debuff.applyStatsTo(newStats);

  // Special case: health must change to reflect new max health
  int healthDecrease = oldMaxHealth - newStats.maxHealth;
  if (healthDecrease != 0) {
    // Alert nearby users to new max health
    server.broadcastToArea(
        location(), {SV_MAX_HEALTH, makeArgs(_name, newStats.maxHealth)});
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
    server.broadcastToArea(
        location(), {SV_MAX_ENERGY, makeArgs(_name, newStats.maxEnergy)});
  }
  int oldEnergy = energy();
  auto newEnergy = oldEnergy - energyDecrease;
  if (newEnergy < 0)
    newEnergy = 0;
  else if (newEnergy > static_cast<int>(newStats.maxEnergy))
    newEnergy = newStats.maxEnergy;
  if (energyDecrease != 0) {
    energy(newEnergy);
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
      makeArgs(newStats.attackTime, newStats.followerLimit, newStats.speed));

  for (const auto &stat : Stats::compositeDefinitions) {
    auto statName = stat.first;
    auto statValue = newStats.getComposite(statName);
    args = makeArgs(args, statValue);
  }

  sendMessage({SV_YOUR_STATS, args});

  stats(newStats);
}

double User::legalMoveDistance(double requestedDistance,
                               double timeElapsed) const {
  const auto TRUST_CLIENTS_WITH_MOVEMENT_SPEED = true;
  if (TRUST_CLIENTS_WITH_MOVEMENT_SPEED) return requestedDistance;

  // Apply limit based on speed
  auto maxLegalDistance =
      min<double>(Server::MAX_TIME_BETWEEN_LOCATION_UPDATES, timeElapsed) /
      1000.0 * stats().speed;
  return min(maxLegalDistance, requestedDistance);
}

bool User::shouldMoveWhereverRequested() const { return isDriving(); }

bool User::knowsConstruction(const std::string &id) const {
  const Server &server = *Server::_instance;
  const ObjectType *objectType = server.findObjectTypeByID(id);
  if (!objectType) return false;
  if (objectType->isKnownByDefault()) return true;
  bool userKnowsConstruction =
      _knownConstructions.find(id) != _knownConstructions.end();
  return userKnowsConstruction;
}

void User::addRecipe(const std::string &id, bool newlyLearned) {
  const Server &server = *Server::_instance;
  auto recipeExists = server.findRecipe(id) != nullptr;
  if (!recipeExists) return;

  _knownRecipes.insert(id);

  if (!newlyLearned) return;
  auto of = std::ofstream{"recipes.log", std::ios_base::app};
  of << _name                        // Player name
     << "," << _knownRecipes.size()  // Recipes known
     << "," << secondsPlayed()       // Time played (s)
     << std::endl;
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

void User::addConstruction(const std::string &id, bool newlyLearned) {
  _knownConstructions.insert(id);

  if (!newlyLearned) return;
  auto of = std::ofstream{"constructions.log", std::ios_base::app};
  of << _name                              // Player name
     << "," << _knownConstructions.size()  // Constructions known
     << "," << secondsPlayed()             // Time played (s)
     << std::endl;
}

void User::sendInfoToClient(const User &targetUser, bool isNew) const {
  const Server &server = Server::instance();
  const Socket &client = targetUser.socket();

  bool isSelf = &targetUser == this;

  // Location
  server.sendMessage(client, {SV_USER_LOCATION, makeLocationCommand()});

  // Hitpoints
  server.sendMessage(client,
                     {SV_MAX_HEALTH, makeArgs(_name, stats().maxHealth)});
  server.sendMessage(client, {SV_PLAYER_HEALTH, makeArgs(_name, health())});

  // Energy
  server.sendMessage(client,
                     {SV_MAX_ENERGY, makeArgs(_name, stats().maxEnergy)});
  server.sendMessage(client, {SV_PLAYER_ENERGY, makeArgs(_name, energy())});

  // Class
  server.sendMessage(
      client, {SV_CLASS, makeArgs(_name, getClass().type().id(), _level)});

  // XP
  if (isSelf) sendXPMessage();

  // City
  const City::Name city = server._cities.getPlayerCity(_name);
  if (!city.empty())
    server.sendMessage(client, {SV_IN_CITY, makeArgs(_name, city)});

  // King?
  if (server._kings.isPlayerAKing(_name))
    server.sendMessage(client, {SV_KING, _name});

  // Gear
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const ServerItem *item = gear(i).type();
    if (item)
      server.sendMessage(
          client, {SV_GEAR, makeArgs(_name, i, item->id(), gear(i).health())});
  }

  // Buffs/debuffs
  for (const auto &buff : buffs()) {
    server.sendMessage(client,
                       {SV_PLAYER_GOT_BUFF, makeArgs(_name, buff.type())});
    if (isSelf)
      server.sendMessage(client, {SV_REMAINING_BUFF_TIME,
                                  makeArgs(buff.type(), buff.timeRemaining())});
  }
  for (const auto &debuff : debuffs()) {
    server.sendMessage(client,
                       {SV_PLAYER_GOT_DEBUFF, makeArgs(_name, debuff.type())});
    if (isSelf)
      server.sendMessage(client,
                         {SV_REMAINING_DEBUFF_TIME,
                          makeArgs(debuff.type(), debuff.timeRemaining())});
  }

  // Vehicle?
  if (isDriving())
    server.sendMessage(client,
                       {SV_VEHICLE_HAS_DRIVER, makeArgs(driving(), _name)});
}

void User::sendInventorySlot(size_t slotIndex) const {
  const auto &slot = _inventory[slotIndex];
  if (!slot.type()) return;  // Is this right?
  const auto isSoulbound = slot.isSoulbound() ? "1" : "0";
  sendMessage(
      {SV_INVENTORY,
       makeArgs(Serial::Inventory(), slotIndex, slot.type()->id(),
                slot.quantity(), slot.health(), isSoulbound, slot.suffix())});
}

void User::sendGearSlot(size_t slotIndex) const {
  const auto &slot = _gear[slotIndex];
  if (!slot.type()) return;
  const auto isSoulbound = slot.isSoulbound() ? "1" : "0";
  sendMessage(
      {SV_INVENTORY,
       makeArgs(Serial::Gear(), slotIndex, slot.type()->id(), slot.quantity(),
                slot.health(), isSoulbound, slot.suffix())});
}

void User::sendSpawnPoint(bool hasChanged) const {
  auto code =
      hasChanged ? SV_YOU_CHANGED_YOUR_SPAWN_POINT : SV_YOUR_SPAWN_POINT;
  sendMessage({code, makeArgs(_respawnPoint.x, _respawnPoint.y)});
}

void User::sendKnownRecipes() const {
  if (_knownRecipes.empty()) return;

  const auto MAX_BATCH_SIZE = 10;
  auto batch = std::set<std::string>{};
  for (const auto &recipeID : _knownRecipes) {
    batch.insert(recipeID);

    if (batch.size() == MAX_BATCH_SIZE) {
      sendKnownRecipesBatch(batch);
      batch.clear();
    }
  }
  if (!batch.empty()) sendKnownRecipesBatch(batch);
}
void User::sendKnownRecipesBatch(const std::set<std::string> &batch) const {
  auto args = makeArgs(batch.size());
  for (auto id : batch) args = makeArgs(args, id);
  sendMessage({SV_YOUR_RECIPES, args});
}

void User::onOutOfRange(const Entity &rhs) const {
  if (rhs.shouldAlwaysBeKnownToUser(*this)) return;
  sendMessage(rhs.outOfRangeMessage());
}

Message User::outOfRangeMessage() const {
  return {SV_USER_OUT_OF_RANGE, name()};
}

void User::moveToSpawnPoint(bool isNewPlayer) {
  Server &server = Server::instance();

  MapPoint newLoc;
  size_t attempts = 0;
  static const size_t MAX_ATTEMPTS = 1000;
  do {
    if (attempts > MAX_ATTEMPTS) {
      server._debug("Failed to find valid spawn location for user",
                    Color::CHAT_ERROR);
      return;
    }
    server._debug << "Attempt #" << ++attempts << " at placing new user"
                  << Log::endl;
    newLoc.x = (randDouble() * 2 - 1) * spawnRadius + _respawnPoint.x;
    newLoc.y = (randDouble() * 2 - 1) * spawnRadius + _respawnPoint.y;
  } while (!server.isLocationValid(newLoc, *this));
  auto oldLoc = location();
  location(newLoc, /* firstInsertion */ isNewPlayer);

  if (isNewPlayer) return;

  server.broadcastToArea(
      oldLoc,
      {SV_USER_LOCATION_INSTANT, makeArgs(name(), location().x, location().y)});
  server.broadcastToArea(
      location(),
      {SV_USER_LOCATION_INSTANT, makeArgs(name(), location().x, location().y)});

  server.sendRelevantEntitiesToUser(*this);
}

void User::onTerrainListChange(const std::string &listID) {
  sendMessage({SV_NEW_TERRAIN_LIST_APPLICABLE, listID});

  // Check that current location is valid
  if (!Server::instance().isLocationValid(location(), *this)) {
    sendMessage(WARNING_BAD_TERRAIN);
    kill();
  }
}

std::set<NPC *> User::findNearbyPets() {
  const auto PET_DEFEND_MASTER_RADIUS = Podes{60}.toPixels();
  auto ret = std::set<NPC *>{};

  Server &server = Server::instance();
  for (auto *entity :
       server.findEntitiesInArea(location(), PET_DEFEND_MASTER_RADIUS)) {
    if (entity->classTag() != 'n') continue;
    auto *npc = dynamic_cast<NPC *>(entity);
    if (!npc->permissions.isOwnedByPlayer(_name)) continue;

    ret.insert(npc);
  }

  return ret;
}

void User::startQuest(const Quest &quest) {
  auto timeRemaining = static_cast<ms_t>(quest.timeLimit * 1000);

  // NOTE: the insertion is the important bit here
  _quests[quest.id] = timeRemaining;

  auto code = quest.canBeCompletedByUser(*this) ? SV_QUEST_CAN_BE_FINISHED
                                                : SV_QUEST_IN_PROGRESS;
  auto &server = Server::instance();
  sendMessage(SV_QUEST_ACCEPTED);
  sendMessage({code, quest.id});

  if (timeRemaining > 0)
    sendMessage({SV_QUEST_TIME_LEFT, makeArgs(quest.id, timeRemaining)});

  for (const auto &itemID : quest.startsWithItems) {
    auto item = server.findItem(itemID);
    if (!item) return;
    giveItem(item);
  }
}

void User::completeQuest(const Quest::ID &id) {
  auto &server = Server::instance();
  const auto quest = server.findQuest(id);

  // Final check before executing: make sure there's room for the rewards
  for (const auto &reward : quest->rewards) {
    if (reward.type == Quest::Reward::ITEM) {
      if (!this->hasRoomFor({reward.id})) {
        sendMessage(WARNING_INVENTORY_FULL);
        return;
      }
    }
  }

  // Remove fetched items
  for (const auto &objective : quest->objectives) {
    if (objective.type == Quest::Objective::FETCH) {
      auto set = ItemSet{};
      set.add(objective.item, objective.qty);
      this->removeItems(set);
    }
  }

  markQuestAsCompleted(id);
  _quests.erase(id);

  // Rewards
  addXP(quest->getXPFor(_level));
  for (const auto &reward : quest->rewards) giveQuestReward(reward);

  for (const auto &unlockedQuestID : quest->otherQuestsWithThisAsPrerequisite) {
    if (canStartQuest(unlockedQuestID))
      sendMessage({SV_QUEST_CAN_BE_STARTED, unlockedQuestID});
  }

  sendMessage({SV_QUEST_COMPLETED, id});
}

void User::giveQuestReward(const Quest::Reward &reward) {
  auto &server = Server::instance();
  switch (reward.type) {
    case Quest::Reward::CONSTRUCTION:
      addConstruction(reward.id);
      server.sendNewBuildsMessage(*this, {reward.id});
      break;
    case Quest::Reward::RECIPE:
      addRecipe(reward.id);
      server.sendNewRecipesMessage(*this, {reward.id});
      break;

    case Quest::Reward::SPELL:
      getClass().teachSpell(reward.id);
      break;

    case Quest::Reward::ITEM: {
      auto rewardItem = server.findItem(reward.id);
      if (!rewardItem) {
        SERVER_ERROR("Quest-reward item doesn't exist: "s + reward.id);
        break;
      }
      giveItem(rewardItem, reward.itemQuantity);
      break;
    }

    case Quest::Reward::NONE:
      break;
  }
}

bool User::hasCompletedQuest(const Quest::ID &id) const {
  auto it = _questsCompleted.find(id);
  return it != _questsCompleted.end();
}

bool User::hasCompletedAllPrerequisiteQuestsOf(const Quest::ID &id) const {
  auto &server = Server::instance();
  const auto *quest = server.findQuest(id);
  for (const auto prereq : quest->prerequisiteQuests) {
    if (!hasCompletedQuest(prereq)) return false;
  }
  return true;
}

void User::abandonQuest(Quest::ID id) {
  _quests.erase(id);

  // Remove quest-exclusive items
  auto &server = Server::instance();
  for (auto i = 0; i != INVENTORY_SIZE; ++i) {
    auto &slot = _inventory[i];
    if (slot.hasItem() && slot.type()->exclusiveToQuest() == id) {
      slot = {};
      server.sendInventoryMessage(*this, i, Serial::Inventory());
    }
  }
  for (auto i = 0; i != GEAR_SLOTS; ++i) {
    auto &slot = _gear[i];
    if (slot.hasItem() && slot.type()->exclusiveToQuest() == id) {
      slot = {};
      server.sendInventoryMessage(*this, i, Serial::Gear());
    }
  }

  sendMessage({SV_QUEST_CAN_BE_STARTED, id});
}

void User::abandonAllQuests() {
  auto copy = _quests;
  for (auto &pair : copy) {
    abandonQuest(pair.first);
  }
}

void User::markQuestAsCompleted(const Quest::ID &id) {
  _questsCompleted.insert(id);
}

void User::markQuestAsStarted(const Quest::ID &id, ms_t timeRemaining) {
  _quests[id] = timeRemaining;
  if (timeRemaining > 0)
    sendMessage({SV_QUEST_TIME_LEFT, makeArgs(id, timeRemaining)});
}

void User::loadBuff(const BuffType &type, ms_t timeRemaining) {
  Object::loadBuff(type, timeRemaining);
  sendMessage({SV_REMAINING_BUFF_TIME, makeArgs(type.id(), timeRemaining)});
}

void User::loadDebuff(const BuffType &type, ms_t timeRemaining) {
  Object::loadDebuff(type, timeRemaining);
  sendMessage({SV_REMAINING_DEBUFF_TIME, makeArgs(type.id(), timeRemaining)});
}

void User::sendBuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(),
                         {SV_PLAYER_GOT_BUFF, makeArgs(_name, buff)});
}

void User::sendDebuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(),
                         {SV_PLAYER_GOT_DEBUFF, makeArgs(_name, buff)});
}

void User::sendLostBuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(),
                         {SV_PLAYER_LOST_BUFF, makeArgs(_name, buff)});
}

void User::sendLostDebuffMsg(const Buff::ID &buff) const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(),
                         {SV_PLAYER_LOST_DEBUFF, makeArgs(_name, buff)});
}

void User::sendXPMessage() const {
  const Server &server = Server::instance();
  sendMessage({SV_YOUR_XP, makeArgs(_xp, XP_PER_LEVEL[_level])});
}

void User::announceLevelUp() const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(), {SV_LEVEL_UP, _name});
}

XP User::appropriateXPForKill(const Entity &victim) const {
  auto xp = XP{};
  auto levelDiff = victim.level() - level();
  if (levelDiff < -9)
    xp = 0;
  else if (levelDiff > 5)
    xp = 150;
  else {
    static auto xpPerLevelDiff = std::unordered_map<int, XP>{
        {-9, 14}, {-8, 27}, {-7, 40}, {-6, 52}, {-5, 62},
        {-4, 72}, {-3, 80}, {-2, 88}, {-1, 95}, {0, 100},
        {1, 105}, {2, 112}, {3, 120}, {4, 129}, {5, 139}};
    xp = xpPerLevelDiff[levelDiff];
  }

  auto rank = victim.type()->rank();
  if (rank == EntityType::ELITE)
    xp *= 5;
  else if (rank == EntityType::BOSS)
    xp *= 10;

  xp = toInt(1.0 * xp / getGroupSize());

  return xp;
}

int User::getGroupSize() const {
  return Server::instance().groups->getGroupSize(name());
}

void User::addXP(XP amount) {
  if (_level == MAX_LEVEL) return;
  _xp += amount;

  Server &server = Server::instance();
  sendMessage({SV_XP_GAIN, amount});

  // Level up if appropriate
  const auto maxXpThisLevel = XP_PER_LEVEL[_level];
  if (_xp >= maxXpThisLevel) {
    levelUp();

    if (_level == MAX_LEVEL)
      _xp = 0;
    else {
      auto surplus = _xp - maxXpThisLevel;
      _xp = surplus;
    }
  }

  sendXPMessage();
}

void User::levelUp() {
  ++_level;
  fillHealthAndEnergy();
  announceLevelUp();

  auto of = std::ofstream{"levels.log", std::ios_base::app};
  of << _name                              // Player name
     << "," << _class.value().type().id()  // Player class
     << "," << _level                      // Level reached
     << "," << secondsPlayed()             // Time played (s)
     << std::endl;
}

bool User::QuestProgress::operator<(const QuestProgress &rhs) const {
  if (quest != rhs.quest) return quest < rhs.quest;
  if (type != rhs.type) return type < rhs.type;
  return ID < rhs.ID;
}

User::ToolSearchResult::ToolSearchResult(DamageOnUse &toolToDamage,
                                         const HasTags &toolWithTags,
                                         const std::string &tag)
    : _type(DAMAGE_ON_USE),
      _toolToDamage(&toolToDamage),
      _toolSpeed(toolWithTags.toolSpeed(tag)) {}

User::ToolSearchResult::ToolSearchResult(Type type, const HasTags &toolWithTags,
                                         const std::string &tag)
    : _type(type), _toolSpeed(toolWithTags.toolSpeed(tag)) {
  if (type == DAMAGE_ON_USE) SERVER_ERROR("Bad tool search");
}

User::ToolSearchResult::ToolSearchResult(Type type) : _type(type) {
  if (type == DAMAGE_ON_USE) SERVER_ERROR("Bad tool search");
}

User::ToolSearchResult::operator bool() const {
  if (_type == NOT_FOUND) return false;
  if (_type == TERRAIN) return true;

  return _toolToDamage && !_toolToDamage->isBroken();
}

void User::ToolSearchResult::use() const {
  if (!_toolToDamage) return;
  _toolToDamage->onUseAsTool();
}
