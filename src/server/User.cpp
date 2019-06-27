#include "User.h"

#include <thread>

#include "../curlUtil.h"
#include "ProgressLock.h"
#include "Server.h"

ObjectType User::OBJECT_TYPE("__clientObjectType__");

MapPoint User::newPlayerSpawn = {};
MapPoint User::postTutorialSpawn = {};
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

      _exploration(Server::instance().map().width(),
                   Server::instance().map().height()),

      _respawnPoint(newPlayerSpawn),

      _driving(0),

      _inventory(INVENTORY_SIZE),
      _gear(GEAR_SLOTS),
      _lastContact(SDL_GetTicks()) {
  if (socket) _socket = *socket;

  if (!OBJECT_TYPE.collides()) {
    OBJECT_TYPE.collisionRect({-5, -2, 10, 4});
  }
  for (size_t i = 0; i != INVENTORY_SIZE; ++i)
    _inventory[i] = std::make_pair<const ServerItem *, size_t>(0, 0);
}

User::User(const Socket &rhs)
    : Object(MapPoint{}), _socket(rhs), _exploration(0, 0) {}

User::User(const MapPoint &loc)
    : Object(loc), _socket(Socket::Empty()), _exploration(0, 0) {}

void User::findRealWorldLocationStatic(User *user) {
  auto &server = Server::instance();
  server.incrementThreadCount();

  auto username = user->name();
  auto result = getLocationFromIP(user->socket().ip());

  if (!Server::hasInstance()) return;
  auto pUser = server.getUserByName(username);
  if (pUser) pUser->_realWorldLocation = result;

  server.decrementThreadCount();
}

void User::findRealWorldLocation() {
  std::thread(findRealWorldLocationStatic, this).detach();
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
  sendMessage(SV_TIME_PLAYED, makeArgs(secondsPlayed()));
}

Message User::teleportMessage(const MapPoint &destination) const {
  return {SV_LOCATION_INSTANT_USER,
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
      if (gear[i].first.type() != item) continue;
      auto spaceAvailable = static_cast<int>(item->stackSize()) -
                            static_cast<int>(gear[i].second);
      if (spaceAvailable > 0) {
        gear[i].second++;
        itemAdded = true;
        break;
      }
    }
    if (itemAdded) continue;  // Next item

    // Inventory pass 1: partial stacks
    for (auto i = 0; i != INVENTORY_SIZE; ++i) {
      if (inventory[i].first.type() != item) continue;

      if (itemAdded) {
        SERVER_ERROR(
            "Trying to find room for an item that has already been added");
        return false;
      }

      auto spaceAvailable = static_cast<int>(item->stackSize()) -
                            static_cast<int>(inventory[i].second);
      if (spaceAvailable > 0) {
        inventory[i].second++;
        itemAdded = true;
        break;
      }
    }
    if (itemAdded) continue;  // Next item

    // Inventory pass 2: empty slots
    for (auto i = 0; i != INVENTORY_SIZE; ++i) {
      auto slotIsEmpty = !inventory[i].first.hasItem();
      if (!slotIsEmpty) continue;

      if (itemAdded) {
        SERVER_ERROR(
            "Trying to find room for an item that has already been added");
        return false;
      }

      inventory[i].first = {item};
      inventory[i].second = 1;
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
  baseStats.weaponDamage = 2;
  baseStats.attackTime = 2000;
  baseStats.speed = 80.0;
  baseStats.stunned = false;
  baseStats.gatherBonus = 0;
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

bool User::alive() const {
  return SDL_GetTicks() - _lastContact <= Server::CLIENT_TIMEOUT;
}

size_t User::giveItem(const ServerItem *item, size_t quantity) {
  auto &server = Server::instance();

  auto remaining = quantity;

  // Gear pass 1: partial stacks
  for (auto i = 0; i != GEAR_SLOTS; ++i) {
    if (_gear[i].first.type() != item) continue;
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
      if (_inventory[i].first.type() != item) continue;

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
      if (_inventory[i].first.hasItem()) continue;

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
      _inventory[i].first = {item};
      _inventory[i].second = qtyInThisSlot;
      server.sendInventoryMessage(*this, i, Server::INVENTORY);
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
      sendMessage(SV_QUEST_PROGRESS, makeArgs(questID, i, progress));
      if (quest->canBeCompletedByUser(*this))
        sendMessage(SV_QUEST_CAN_BE_FINISHED, questID);
    }
  }

  auto quantityGiven = quantity - remaining;
  if (quantityGiven > 0) {
    ProgressLock::triggerUnlocks(*this, ProgressLock::ITEM, item);
    sendMessage(SV_RECEIVED_ITEM, makeArgs(item->id(), quantityGiven));
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
    // resetAttackTimer();
    _shouldSuppressAmmoWarnings = false;
  } else {
    sendMessage(WARNING_ACTION_INTERRUPTED);
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
  if (!obj->type()) {
    SERVER_ERROR("Can't gather from object with no type");
    return;
  }
  _actionTime = obj->objType().gatherTime();
}

void User::beginCrafting(const SRecipe &recipe) {
  _action = CRAFT;
  _actionRecipe = &recipe;
  _actionTime = recipe.time();
}

void User::beginConstructing(const ObjectType &obj, const MapPoint &location,
                             bool cityOwned, size_t slot) {
  _action = CONSTRUCT;
  _actionObjectType = &obj;
  _actionTime = obj.constructionTime();
  _actionSlot = slot;
  _actionLocation = location;
  _actionOwnedByCity = cityOwned;
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

bool User::hasItems(const ItemSet &items) const {
  ItemSet remaining = items;
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    const auto &invSlot = _inventory[i];
    remaining.remove(invSlot.first.type(), invSlot.second);
    if (remaining.isEmpty()) return true;
  }
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const auto &gearSlot = _gear[i];
    remaining.remove(gearSlot.first.type(), gearSlot.second);
    if (remaining.isEmpty()) return true;
  }
  return false;
}

bool User::hasItems(const std::string &tag, size_t quantity) const {
  auto remaining = quantity;
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    const auto &invSlot = _inventory[i];
    if (!invSlot.first.hasItem()) continue;
    if (invSlot.first.type()->isTag(tag)) {
      if (invSlot.second >= remaining) return true;
      remaining -= invSlot.second;
    }
  }
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const auto &gearSlot = _gear[i];
    if (!gearSlot.first.hasItem()) continue;
    if (gearSlot.first.type()->isTag(tag)) {
      if (gearSlot.second >= remaining) return true;
      remaining -= gearSlot.second;
    }
  }
  return false;
}

User::ToolSearchResult User::findTool(const std::string &tagName) {
  // Check gear
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    auto &slot = _gear[i].first;
    if (slot.hasItem() && slot.type()->isTag(tagName)) return {slot};
  }

  // Check inventory
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    auto &slot = _inventory[i].first;
    if (slot.hasItem() && slot.type()->isTag(tagName)) return {slot};
  }

  // Check nearby terrain
  Server &server = *Server::_instance;
  auto nearbyTerrain = server.map().terrainTypesOverlapping(
      collisionRect(), Server::ACTION_DISTANCE);
  for (char terrainType : nearbyTerrain) {
    if (server.terrainType(terrainType)->tag() == tagName)
      return {ToolSearchResult::TERRAIN};
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

      return {ToolSearchResult::OBJECT};
    }

  return {ToolSearchResult::NOT_FOUND};
}

bool User::checkAndDamageTools(const std::set<std::string> &tags) {
  auto toolsFound = std::vector<ToolSearchResult>{};
  for (const std::string &tagName : tags) {
    auto result = findTool(tagName);
    if (!result) return false;
    toolsFound.push_back(result);
  }

  // At this point, all tools were found and true will be returned.  Only now
  // should all tools be damaged.
  for (const auto &tool : toolsFound) tool.use();

  return true;
}

bool User::checkAndDamageTool(const std::string &tag) {
  auto tool = findTool(tag);
  if (!tool) return false;
  tool.use();
  return true;
}

void User::clearInventory() {
  const Server &server = *Server::_instance;
  for (auto i = 0; i != INVENTORY_SIZE; ++i)
    if (_inventory[i].first.hasItem()) {
      _inventory[i].first = {};
      _inventory[i].second = 0;
      server.sendMessage(socket(), SV_INVENTORY,
                         makeArgs(Server::INVENTORY, i, "", 0, 0));
    }
}

void User::clearGear() {
  const Server &server = *Server::_instance;
  for (auto i = 0; i != GEAR_SLOTS; ++i)
    if (_gear[i].first.hasItem()) {
      _gear[i].first = {};
      _gear[i].second = 0;
      server.sendMessage(socket(), SV_INVENTORY,
                         makeArgs(Server::GEAR, i, "", 0, 0));
    }
}

static void removeItemsFrom(ItemSet &remaining, ServerItem::vect_t &container,
                            std::set<size_t> &slotsChanged) {
  slotsChanged = {};
  for (size_t i = 0; i != container.size(); ++i) {
    auto &slot = container[i];
    auto &itemType = slot.first;
    auto &qty = slot.second;
    if (remaining.contains(itemType.type())) {
      size_t itemsToRemove = min(qty, remaining[itemType.type()]);
      remaining.remove(itemType.type(), itemsToRemove);
      qty -= itemsToRemove;
      if (qty == 0) itemType = {};
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

  if (!remaining.isEmpty()) {
    SERVER_ERROR("Failed to remove all necessary items from user");
  }
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
    if (!itemType.hasItem()) continue;
    if (itemType.type()->isTag(tag)) {
      size_t itemsToRemove = min(qty, remaining);
      remaining -= itemsToRemove;

      qty -= itemsToRemove;
      if (qty == 0) itemType = {};

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
    if (pair.first.type() == item) count += pair.second;

  for (auto &pair : _inventory)
    if (pair.first.type() == item) count += pair.second;

  return count;
}

void User::sendMessage(MessageCode msgCode, const std::string &args) const {
  if (!_socket.hasValue()) return;
  const Server &server = Server::instance();
  server.sendMessage(_socket.value(), msgCode, args);
}

void User::update(ms_t timeElapsed) {
  regen(timeElapsed);

  // Quests
  auto questsToAbandon = std::set<std::string>{};
  for (auto &pair : _quests) {
    // No time limit
    if (pair.second == 0) continue;

    // Time has run out
    if (timeElapsed >= pair.second) {
      questsToAbandon.insert(pair.first);
      sendMessage(SV_QUEST_FAILED, pair.first);
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
      if (!_actionObject->contents().isEmpty())
        server.gatherObject(_actionObject->serial(), *this);
      break;

    case CRAFT: {
      if (!hasRoomToCraft(*_actionRecipe)) {
        sendMessage(WARNING_INVENTORY_FULL);
        cancelAction();
        Entity::update(timeElapsed);
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
      if (_actionSlot ==
          INVENTORY_SIZE)  // Constructing an object without an item
        break;

      // Remove item from user's inventory
      auto &slot = _inventory[_actionSlot];
      if (slot.first.type()->constructsObject() != _actionObjectType) {
        SERVER_ERROR("Trying to construct object from an invalid item");
        break;
      }
      --slot.second;
      if (slot.second == 0) slot.first = {};
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
        sendMessage(WARNING_INVENTORY_FULL);
        cancelAction();
        break;
      }
      // Give user his item
      giveItem(item);
      // Remove object
      server.removeEntity(*_actionObject);
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

bool User::hasRoomToCraft(const SRecipe &recipe) const {
  size_t slotsFreedByMaterials = 0;
  ItemSet remainingMaterials = recipe.materials();
  ServerItem::vect_t inventoryCopy = _inventory;
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    auto &invSlot = inventoryCopy[i];
    if (remainingMaterials.contains(invSlot.first.type())) {
      size_t itemsToRemove =
          min(invSlot.second, remainingMaterials[invSlot.first.type()]);
      remainingMaterials.remove(invSlot.first.type(), itemsToRemove);
      inventoryCopy[i].second -= itemsToRemove;
      if (inventoryCopy[i].second == 0) inventoryCopy[i].first = {};
      if (remainingMaterials.isEmpty()) break;
    }
  }
  return vectHasSpace(inventoryCopy, toServerItem(recipe.product()),
                      recipe.quantity());
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
  sendMessage(SV_HOTBAR, args);
}

void User::onMove() {
  auto &server = Server::instance();

  // Explore map
  auto chunk = _exploration.getChunk(location());
  auto newlyExploredChunks = _exploration.explore(chunk);
  for (const auto &chunk : newlyExploredChunks)
    _exploration.sendSingleChunk(socket(), chunk);

  // Get buffs from objects
  auto buffsToAdd = std::map<const BuffType *, Entity *>{};
  for (auto *entity : server.findEntitiesInArea(location())) {
    const Object *pObj = dynamic_cast<const Object *>(entity);
    if (pObj == nullptr) continue;
    if (!pObj->permissions().doesUserHaveAccess(_name)) continue;
    if (pObj->isBeingBuilt()) continue;
    const auto &objType = pObj->objType();
    if (!objType.grantsBuff()) continue;
    if (distance(pObj->collisionRect(), collisionRect()) > objType.buffRadius())
      continue;

    buffsToAdd[objType.buffGranted()] = entity;
  }

  // Remove any disqualified pre-existing object buffs
  auto buffsToRemove = std::set<std::string>{};
  for (const auto &currentlyActiveBuff : buffs()) {
    const auto *buffType = server.findBuff(currentlyActiveBuff.type());

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

bool User::canBeAttackedBy(const User &user) const {
  const Server &server = *Server::_instance;
  return server._wars.isAtWar({_name}, {user._name});
}

px_t User::attackRange() const {
  const auto weapon = _gear[Item::WEAPON_SLOT].first;
  if (!weapon.hasItem()) return Object::attackRange();
  return weapon.type()->weaponRange();
}

CombatResult User::generateHitAgainst(const Entity &target, CombatType type,
                                      SpellSchool school, px_t range) const {
  const auto BASE_MISS_CHANCE = Percentage{10};

  auto levelDiff = target.level() - level();
  auto modifierFromLevelDiff = levelDiff * 3;

  auto roll = rand() % 100;

  // Miss
  auto missChance = BASE_MISS_CHANCE - stats().hit + modifierFromLevelDiff;
  missChance = max(0, missChance);
  if (combatTypeCanHaveOutcome(type, MISS, school, range)) {
    if (roll < missChance) return MISS;
    roll -= missChance;
  }

  // Dodge
  auto dodgeChance = target.stats().dodge + modifierFromLevelDiff;
  dodgeChance = max(0, dodgeChance);
  if (combatTypeCanHaveOutcome(type, DODGE, school, range)) {
    if (roll < dodgeChance) return DODGE;
    roll -= dodgeChance;
  }

  // Block
  auto blockChance = target.stats().block + modifierFromLevelDiff;
  blockChance = max(0, blockChance);
  if (target.canBlock() &&
      combatTypeCanHaveOutcome(type, BLOCK, school, range)) {
    if (roll < blockChance) return BLOCK;
    roll -= blockChance;
  }

  // Crit
  auto critChance =
      stats().crit - target.stats().critResist - modifierFromLevelDiff;
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
  if (!offhandItem.hasItem()) return false;
  return offhandItem.type()->isTag("shield");
}

SpellSchool User::school() const {
  auto weapon = _gear[Item::WEAPON_SLOT].first;
  if (!weapon.hasItem()) return {};
  return weapon.type()->stats().weaponSchool;
}

double User::combatDamage() const {
  if (stats().weaponSchool == SpellSchool::PHYSICAL)
    return stats().weaponDamage + stats().physicalDamage;
  else
    return stats().weaponDamage + stats().magicDamage;
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
  setTargetAndAttack(nullptr);
  cancelAction();
  _action = NO_ACTION;

  sendMessage(SV_YOU_DIED);

  // Handle respawn etc.
  moveToSpawnPoint();

  auto talentLost = _class.value().loseARandomLeafTalent();
  if (!talentLost.empty()) {
    const Server &server = *Server::_instance;
    sendMessage(SV_LOST_TALENT, talentLost);
  }

  health(stats().maxHealth);
  energy(stats().maxEnergy);
  onHealthChange();
  onEnergyChange();
}

void User::onNewOwnedObject(const ObjectType &type) const {
  if (type.isPlayerUnique())
    this->_playerUniqueCategoriesOwned.insert(type.playerUniqueCategory());
}

void User::onDestroyedOwnedObject(const ObjectType &type) const {
  if (!type.isPlayerUnique()) return;
  this->_playerUniqueCategoriesOwned.erase(type.playerUniqueCategory());
}

void User::onAttackedBy(Entity &attacker, Threat threat) {
  cancelAction();

  auto armourSlotToUse = Item::getRandomArmorSlot();
  auto armourWasDamaged = _gear[armourSlotToUse].first.onUse();

  if (armourWasDamaged) sendInventorySlot(armourSlotToUse);

  Object::onAttackedBy(attacker, threat);
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
      sendMessage(SV_QUEST_PROGRESS, makeArgs(questID, i, progress));
      if (quest->canBeCompletedByUser(*this))
        sendMessage(SV_QUEST_CAN_BE_FINISHED, questID);

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
  const auto &gearSlot = _gear[Item::WEAPON_SLOT];

  auto hasWeapon = gearSlot.first.hasItem();
  if (!hasWeapon) return true;

  auto weapon = gearSlot.first.type();
  if (!weapon->usesAmmo()) return true;

  auto ammoType = weapon->weaponAmmo();
  auto itemSet = ItemSet{};
  itemSet.add(ammoType, 1);
  if (this->hasItems(itemSet)) return true;

  auto ammoID = weapon->weaponAmmo()->id();
  if (!_shouldSuppressAmmoWarnings) {
    sendMessage(WARNING_OUT_OF_AMMO, ammoID);
    _shouldSuppressAmmoWarnings = true;
  }
  return false;
}

void User::onCanAttack() { _shouldSuppressAmmoWarnings = false; }

void User::onAttack() {
  // Remove ammo if ranged weapon
  auto &weapon = _gear[Item::WEAPON_SLOT].first;
  if (weapon.hasItem() && weapon.type()->weaponAmmo()) {
    auto ammo = ItemSet{};
    ammo.add(weapon.type()->weaponAmmo());
    removeItems(ammo);

    // If the weapon itself is used as ammo and there are none left
    if (!weapon.hasItem()) updateStats();
  }

  if (weapon.hasItem()) {
    auto weaponTookDamage = weapon.onUse();
    if (weaponTookDamage) sendInventorySlot(Item::WEAPON_SLOT);
    if (weapon.isBroken()) updateStats();
  }
}

void User::onSpellcast(const Spell::ID &id, const Spell &spell) {
  if (spell.cooldown() != 0)
    sendMessage(SV_SPELL_COOLING_DOWN, makeArgs(id, spell.cooldown()));

  addQuestProgress(Quest::Objective::CAST_SPELL, id);
}

void User::sendRangedHitMessageTo(const User &userToInform) const {
  if (!target()) return;
  auto weapon = _gear[Item::WEAPON_SLOT].first.type();
  if (!weapon) return;

  Server &server = *Server::_instance;
  server.sendMessage(userToInform.socket(), SV_RANGED_WEAPON_HIT,
                     makeArgs(weapon->id(), location().x, location().y,
                              target()->location().x, target()->location().y));
}

void User::sendRangedMissMessageTo(const User &userToInform) const {
  if (!target()) return;
  auto weapon = _gear[Item::WEAPON_SLOT].first.type();
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
    const auto &item = _gear[i].first;
    if (!item.hasItem()) continue;

    if (item.isBroken()) continue;

    newStats &= item.type()->stats();
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
  sendMessage(SV_YOUR_STATS, args);

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
    const ServerItem *item = gear(i).first.type();
    if (item != nullptr)
      server.sendMessage(
          client, SV_GEAR,
          makeArgs(_name, i, item->id(), gear(i).first.health()));
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

void User::sendInventorySlot(size_t slotIndex) const {
  const auto &slot = _gear[slotIndex];
  const auto &item = slot.first;
  if (!item.type()) return;
  sendMessage(SV_INVENTORY, makeArgs(Server::GEAR, slotIndex, item.type()->id(),
                                     slot.second, item.health()));
}

void User::onOutOfRange(const Entity &rhs) const {
  if (rhs.shouldAlwaysBeKnownToUser(*this)) return;

  auto message = rhs.outOfRangeMessage();
  sendMessage(message.code, message.args);
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

  server.broadcastToArea(oldLoc, SV_LOCATION_INSTANT_USER,
                         makeArgs(name(), location().x, location().y));
  server.broadcastToArea(location(), SV_LOCATION_INSTANT_USER,
                         makeArgs(name(), location().x, location().y));

  server.sendRelevantEntitiesToUser(*this);
}

void User::onTerrainListChange(const std::string &listID) {
  sendMessage(SV_NEW_TERRAIN_LIST_APPLICABLE, listID);

  // Check that current location is valid
  if (!Server::instance().isLocationValid(location(), *this)) {
    sendMessage(WARNING_BAD_TERRAIN);
    kill();
  }
}

void User::startQuest(const Quest &quest) {
  auto timeRemaining = static_cast<ms_t>(quest.timeLimit * 1000);

  // NOTE: the insertion is the important bit here
  _quests[quest.id] = timeRemaining;

  auto message = quest.canBeCompletedByUser(*this) ? SV_QUEST_CAN_BE_FINISHED
                                                   : SV_QUEST_IN_PROGRESS;
  auto &server = Server::instance();
  sendMessage(SV_QUEST_ACCEPTED);
  sendMessage(message, quest.id);

  if (timeRemaining > 0)
    sendMessage(SV_QUEST_TIME_LEFT, makeArgs(quest.id, timeRemaining));

  for (const auto &itemID : quest.startsWithItems) {
    auto item = server.findItem(itemID);
    if (!item) return;
    giveItem(item);
  }
}

void User::completeQuest(const Quest::ID &id) {
  auto &server = Server::instance();
  const auto quest = server.findQuest(id);

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
  addXP(250);
  giveQuestReward(quest->reward);

  for (const auto &unlockedQuestID : quest->otherQuestsWithThisAsPrerequisite) {
    if (canStartQuest(unlockedQuestID))
      sendMessage(SV_QUEST_CAN_BE_STARTED, unlockedQuestID);
  }

  sendMessage(SV_QUEST_COMPLETED, id);
}

void User::giveQuestReward(const Quest::Reward &reward) {
  auto &server = Server::instance();
  switch (reward.type) {
    case Quest::Reward::CONSTRUCTION:
      addConstruction(reward.id);
      server.sendNewBuildsMessage(*this, {reward.id});
      break;

    case Quest::Reward::SPELL:
      getClass().teachSpell(reward.id);
      break;

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
    auto &slotPair = _inventory[i];
    if (slotPair.first.hasItem() &&
        slotPair.first.type()->exclusiveToQuest() == id) {
      slotPair.first = {};
      slotPair.second = 0;
      server.sendInventoryMessage(*this, i, Server::INVENTORY);
    }
  }
  for (auto i = 0; i != GEAR_SLOTS; ++i) {
    auto &slotPair = _gear[i];
    if (slotPair.first.hasItem() &&
        slotPair.first.type()->exclusiveToQuest() == id) {
      slotPair.first = {};
      slotPair.second = 0;
      server.sendInventoryMessage(*this, i, Server::GEAR);
    }
  }

  sendMessage(SV_QUEST_CAN_BE_STARTED, id);
}

void User::markQuestAsCompleted(const Quest::ID &id) {
  _questsCompleted.insert(id);
}

void User::markQuestAsStarted(const Quest::ID &id, ms_t timeRemaining) {
  _quests[id] = timeRemaining;
  if (timeRemaining > 0)
    sendMessage(SV_QUEST_TIME_LEFT, makeArgs(id, timeRemaining));
}

void User::loadBuff(const BuffType &type, ms_t timeRemaining) {
  Object::loadBuff(type, timeRemaining);
  sendMessage(SV_REMAINING_BUFF_TIME, makeArgs(type.id(), timeRemaining));
}

void User::loadDebuff(const BuffType &type, ms_t timeRemaining) {
  Object::loadDebuff(type, timeRemaining);
  sendMessage(SV_REMAINING_DEBUFF_TIME, makeArgs(type.id(), timeRemaining));
}

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
  sendMessage(SV_XP, makeArgs(_xp, XP_PER_LEVEL[_level]));
}

void User::announceLevelUp() const {
  const Server &server = Server::instance();
  server.broadcastToArea(location(), SV_LEVEL_UP, _name);
}

void User::addXP(XP amount) {
  if (_level == MAX_LEVEL) return;
  _xp += amount;

  Server &server = Server::instance();
  sendMessage(SV_XP_GAIN, makeArgs(amount));

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
}

bool User::QuestProgress::operator<(const QuestProgress &rhs) const {
  if (quest != rhs.quest) return quest < rhs.quest;
  if (type != rhs.type) return type < rhs.type;
  return ID < rhs.ID;
}

User::ToolSearchResult::ToolSearchResult(Type type) : _type(type) {
  if (type == ITEM) SERVER_ERROR("Bad tool search");
}

void User::ToolSearchResult::use() const {
  switch (_type) {
    case ITEM:
      _item->onUse();
      break;
  }
}
