#include <algorithm>
#include <set>

#include "../MessageParser.h"
#include "../versionUtil.h"
#include "DroppedItem.h"
#include "Groups.h"
#include "ProgressLock.h"
#include "Server.h"
#include "Vehicle.h"
#include "objects/Deconstruction.h"

#define READ_ARGS(...)                                      \
  auto messageWasWellFormed = parser.readArgs(__VA_ARGS__); \
  if (!messageWasWellFormed) return

#define CHECK_NO_ARGS                                                   \
  auto messageWasWellFormed = parser.getLastDelimiterRead() == MSG_END; \
  if (!messageWasWellFormed) return

#define HANDLE_MESSAGE(CODE)                                           \
  template <>                                                          \
  void Server::handleMessage<(CODE)>(const Socket &client, User &user, \
                                     MessageParser &parser)

#define RETURN_WITH(MSG)      \
  {                           \
    sendMessage(client, MSG); \
    return;                   \
  }

HANDLE_MESSAGE(CL_REPORT_BUG) {
  std::string bugDescription;
  READ_ARGS(bugDescription);

  auto bugFile = std::ofstream{"bugs.log", std::ofstream::app};
  bugFile << user.name() << ": " << bugDescription << std::endl;
}

HANDLE_MESSAGE(CL_PING) {
  ms_t timeSent;
  READ_ARGS(timeSent);

  sendMessage(client, {SV_PING_REPLY, timeSent});
}

HANDLE_MESSAGE(CL_LOGIN_EXISTING) {
  std::string username, passwordHash, clientVersion;
  READ_ARGS(username, passwordHash, clientVersion);

#ifndef _DEBUG
  if (clientVersion != version()) {
    sendMessage(client, {WARNING_WRONG_VERSION, version()});
    return;
  }
#endif

  if (!isUsernameValid(username)) RETURN_WITH(WARNING_INVALID_USERNAME)

  auto userIsAlreadyLoggedIn = _usersByName.count(username) == 1;
  if (userIsAlreadyLoggedIn) RETURN_WITH(WARNING_DUPLICATE_USERNAME)

  // Check that user exists
  auto userFile = _userFilesPath + username + ".usr";
  if (!fileExists(userFile)) {
#ifndef _DEBUG
    RETURN_WITH(WARNING_USER_DOESNT_EXIST)
#else
    // Allow quick, auto account creation in debug mode
    addUser(client, username, passwordHash, _classes.begin()->first);
    return;
#endif
  }

  auto xr = XmlReader::FromFile(_userFilesPath + username + ".usr");
  auto elem = xr.findChild("general");
  auto savedPwHash = ""s;
  xr.findAttr(elem, "passwordHash", savedPwHash);
  if (savedPwHash != passwordHash) {
    sendMessage(client, WARNING_WRONG_PASSWORD);
    return;
  }

  addUser(client, username, passwordHash);
}

HANDLE_MESSAGE(CL_LOGIN_NEW) {
  std::string name, pwHash, classID, clientVersion;
  READ_ARGS(name, pwHash, classID, clientVersion);

#ifndef _DEBUG
  // Check that version matches
  if (clientVersion != version()) {
    sendMessage(client, {WARNING_WRONG_VERSION, version()});
    return;
  }
#endif

  if (!isUsernameValid(name)) RETURN_WITH(WARNING_INVALID_USERNAME)

  // Check that user doesn't exist
  auto userFile = _userFilesPath + name + ".usr";
  if (fileExists(userFile)) RETURN_WITH(WARNING_NAME_TAKEN)

  addUser(client, name, pwHash, classID);
}

HANDLE_MESSAGE(CL_REQUEST_TIME_PLAYED) {
  CHECK_NO_ARGS;

  user.sendTimePlayed();
}

HANDLE_MESSAGE(CL_SKIP_TUTORIAL) {
  CHECK_NO_ARGS;

  if (!user.isInTutorial()) return;

  user.exploration.unexploreAll(user.socket());
  user.setSpawnPointToPostTutorial();
  user.sendSpawnPoint(false);
  user.moveToSpawnPoint();

  auto &userClass = user.getClass();
  userClass.unlearnAll();
  auto UTILITY_SPELLS = std::map<std::string, std::string>{
      {"Athlete", "sprint"}, {"Scholar", "blink"}, {"Zealot", "waterWalking"}};
  auto spellToTeach = UTILITY_SPELLS[userClass.type().id()];
  userClass.teachSpell(spellToTeach);

  user.abandonAllQuests();

  user.clearInventory();
  user.clearGear();

  if (!user.knowsConstruction("tutFire"))
    user.sendMessage({SV_NEW_CONSTRUCTIONS, makeArgs(1, "fire")});
  else
    user.sendMessage({SV_CONSTRUCTIONS, makeArgs(1, "fire")});
  user.removeConstruction("tutFire");
  user.addConstruction("fire");

  if (!user.knowsRecipe("cookedMeat")) {
    user.addRecipe("cookedMeat");
    user.sendMessage({SV_NEW_RECIPES, makeArgs(1, "cookedMeat")});
  }

  user.updateStats();

  removeAllObjectsOwnedBy({Permissions::Owner::PLAYER, user.name()});

  user.markTutorialAsCompleted();
}

HANDLE_MESSAGE(CL_LOCATION) {
  double x, y;
  READ_ARGS(x, y);

  if (user.isWaitingForDeathAcknowledgement) return;

  if (user.isStunned()) {
    client.sendMessage({SV_LOCATION, makeArgs(user.name(), user.location().x,
                                              user.location().y)});
    return;
  }

  if (user.action() != User::ATTACK) user.cancelAction();
  if (user.isDriving()) {
    // Move vehicle and user together
    auto vehicleSerial = user.driving();
    auto &vehicle = *_entities.find<Vehicle>(vehicleSerial);
    vehicle.moveLegallyTowards({x, y});
    auto locationWasCorrected = vehicle.location() != MapPoint{x, y};
    auto shouldSendUpdate = locationWasCorrected ? Entity::AlwaysSendUpdate
                                                 : Entity::OnServerCorrection;
    user.moveLegallyTowards(vehicle.location(), shouldSendUpdate);
  } else {
    user.moveLegallyTowards({x, y});
  }
}

HANDLE_MESSAGE(CL_CRAFT) {
  auto recipeID = ""s;
  READ_ARGS(recipeID);

  if (user.isStunned()) RETURN_WITH(WARNING_STUNNED)
  user.cancelAction();
  const std::set<SRecipe>::const_iterator it = _recipes.find(recipeID);
  if (!user.knowsRecipe(recipeID)) RETURN_WITH(ERROR_UNKNOWN_RECIPE)
  ItemSet remaining;
  if (!user.hasItems(it->materials())) RETURN_WITH(WARNING_NEED_MATERIALS)

  // Tool check must be the last check, as it damages the tools.
  auto speed = user.checkAndDamageToolsAndGetSpeed(it->tools());
  auto userHasRequiredTools = speed != 0;
  if (!userHasRequiredTools) RETURN_WITH(WARNING_NEED_TOOLS)

  user.beginCrafting(*it, speed);
}

HANDLE_MESSAGE(CL_CONSTRUCT) {
  auto id = ""s;
  auto x = double{}, y = double{};
  READ_ARGS(id, x, y);

  user.tryToConstruct(id, {x, y}, Permissions::Owner::PLAYER);
}

HANDLE_MESSAGE(CL_CONSTRUCT_FOR_CITY) {
  auto id = ""s;
  auto x = double{}, y = double{};
  READ_ARGS(id, x, y);

  user.tryToConstruct(id, {x, y}, Permissions::Owner::CITY);
}

HANDLE_MESSAGE(CL_TRADE) {
  auto serial = Serial{};
  auto slot = size_t{0};
  READ_ARGS(serial, slot);

  if (user.isStunned()) RETURN_WITH(WARNING_STUNNED)

  // Check that merchant slot is valid
  Object *obj = _entities.find<Object>(serial);
  if (!isEntityInRange(client, user, obj)) return;
  if (obj->isBeingBuilt()) RETURN_WITH(ERROR_UNDER_CONSTRUCTION)
  size_t slots = obj->objType().merchantSlots();
  if (slots == 0)
    RETURN_WITH(ERROR_NOT_MERCHANT)
  else if (slot >= slots)
    RETURN_WITH(ERROR_INVALID_MERCHANT_SLOT)
  const MerchantSlot &mSlot = obj->merchantSlot(slot);
  if (!mSlot) RETURN_WITH(ERROR_INVALID_MERCHANT_SLOT)

  // Check that user has price
  auto priceCheck = containerHasEnoughToTrade(user.inventory(), mSlot.price());
  switch (priceCheck) {
    case ServerItem::ITEMS_MISSING:
      RETURN_WITH(WARNING_NO_PRICE)
    case ServerItem::ITEMS_SOULBOUND:
      RETURN_WITH(WARNING_PRICE_IS_SOULBOUND)
    case ServerItem::ITEMS_BROKEN:
      RETURN_WITH(WARNING_PRICE_IS_BROKEN)
  }

  // Check that user has inventory space
  if (!obj->hasContainer() && !obj->objType().bottomlessMerchant())
    RETURN_WITH(ERROR_NO_INVENTORY)
  auto wareItem = toServerItem(mSlot.wareItem);
  auto priceItem = toServerItem(mSlot.priceItem);
  if (!vectHasSpaceAfterRemovingItems(user.inventory(), wareItem, mSlot.wareQty,
                                      priceItem, mSlot.priceQty))
    RETURN_WITH(WARNING_INVENTORY_FULL)

  bool bottomless = obj->objType().bottomlessMerchant();
  if (!bottomless) {
    // Check that object has items in stock
    auto wareCheck =
        containerHasEnoughToTrade(obj->container().raw(), mSlot.ware());
    switch (wareCheck) {
      case ServerItem::ITEMS_MISSING:
        RETURN_WITH(WARNING_NO_WARE)
      case ServerItem::ITEMS_SOULBOUND:
        RETURN_WITH(WARNING_WARE_IS_SOULBOUND)
      case ServerItem::ITEMS_BROKEN:
        RETURN_WITH(WARNING_WARE_IS_BROKEN)
    }

    // Check that object has inventory space
    auto priceItem = toServerItem(mSlot.priceItem);
    if (!vectHasSpaceAfterRemovingItems(obj->container().raw(), priceItem,
                                        mSlot.priceQty, wareItem,
                                        mSlot.wareQty))
      RETURN_WITH(WARNING_MERCHANT_INVENTORY_FULL)
  }

  // Take price from user
  user.removeItems(mSlot.price());

  if (!bottomless) {
    // Take ware from object
    obj->container().removeItems(mSlot.ware());

    // Give price to object
    obj->container().addItems(toServerItem(mSlot.priceItem), mSlot.priceQty);
  }

  // Give ware to user
  user.giveItem(wareItem, mSlot.wareQty);
}

HANDLE_MESSAGE(CL_DROP) {
  Serial serial;
  size_t slot;
  READ_ARGS(serial, slot);

  if (user.isStunned()) RETURN_WITH(WARNING_STUNNED)

  auto info = getContainer(user, serial);
  if (info.hasWarning()) RETURN_WITH(info.warning);

  if (slot >= info.container->size()) RETURN_WITH(ERROR_INVALID_SLOT)

  auto &containerSlot = (*info.container)[slot];

  const auto &itemInstance = containerSlot.first;
  const auto &item = *containerSlot.first.type();
  const auto quantity = containerSlot.second;
  if (quantity == 0) return;

  if (itemInstance.isSoulbound()) RETURN_WITH(WARNING_ITEM_IS_BOUND);

  auto dropLocation = MapPoint{};
  const auto MAX_ATTEMPTS = 50;
  for (auto attempt = 0; attempt != MAX_ATTEMPTS; ++attempt) {
    dropLocation =
        getRandomPointInCircle(user.location(), Server::ACTION_DISTANCE);
    if (isLocationValid(dropLocation, DroppedItem::TYPE)) {
      addEntity(new DroppedItem(item, quantity, dropLocation));
      break;
    }

    if (attempt == MAX_ATTEMPTS - 1) {
      RETURN_WITH(WARNING_NOWHERE_TO_DROP_ITEM);
    }
  }

  containerSlot.first = {};
  containerSlot.second = 0;

  // Alert relevant users
  if (serial.isInventory() || serial.isGear())
    sendInventoryMessage(user, slot, serial);
  else
    info.object->tellRelevantUsersAboutInventorySlot(slot);
}

HANDLE_MESSAGE(CL_PICK_UP_DROPPED_ITEM) {
  Serial serial;
  READ_ARGS(serial);
  auto entity = findEntityBySerial(serial);
  if (!entity) RETURN_WITH(WARNING_DOESNT_EXIST);
  if (distance(entity->collisionRect(), user.collisionRect()) > ACTION_DISTANCE)
    RETURN_WITH(WARNING_TOO_FAR);
  auto asDroppedItem = dynamic_cast<DroppedItem *>(entity);
  if (!asDroppedItem) return;

  asDroppedItem->getPickedUpBy(user);
}

HANDLE_MESSAGE(CL_SWAP_ITEMS) {
  Serial obj1, obj2;
  size_t slot1, slot2;
  READ_ARGS(obj1, slot1, obj2, slot2);

  if (user.isStunned()) RETURN_WITH(WARNING_STUNNED)
  user.cancelAction();

  auto from = getContainer(user, obj1);
  if (from.hasWarning()) RETURN_WITH(from.warning)
  auto to = getContainer(user, obj2);
  if (to.hasWarning()) RETURN_WITH(from.warning)

  if (!from.container) RETURN_WITH(ERROR_NO_INVENTORY);

  bool isConstructionMaterial = false;
  if (to.object && to.object->isBeingBuilt() && slot2 == 0)
    isConstructionMaterial = true;
  if (!isConstructionMaterial && !to.container) RETURN_WITH(ERROR_NO_INVENTORY)

  auto maxValidToSlot = isConstructionMaterial ? 0 : (to.container->size() - 1);
  if (slot1 >= from.container->size() || slot2 > maxValidToSlot)
    RETURN_WITH(ERROR_INVALID_SLOT)

  auto &slotFrom = (*from.container)[slot1];
  auto &fromItem = slotFrom.first;
  if (!fromItem.hasItem()) {
    SERVER_ERROR("Attempting to move nonexistent item");
    return;
  }

  if (isConstructionMaterial) {
    if (fromItem.isBroken()) RETURN_WITH(WARNING_BROKEN_ITEM)

    const auto *materialType = fromItem.type();
    size_t qtyInSlot = slotFrom.second,
           qtyNeeded = to.object->remainingMaterials()[materialType],
           qtyToTake = min(qtyInSlot, qtyNeeded);

    if (qtyNeeded == 0) RETURN_WITH(WARNING_WRONG_MATERIAL)

    auto itemToReturn = materialType->returnsOnConstruction();
    if (itemToReturn) {
      auto itemsToRemove = ItemSet{};
      itemsToRemove.add(materialType, qtyToTake);
      auto itemsToAdd = ItemSet{};
      itemsToAdd.add(itemToReturn, 1);
      if (!user.hasRoomToRemoveThenAdd(itemsToRemove, itemsToAdd))
        RETURN_WITH(WARNING_INVENTORY_FULL)
    }

    // Remove from object requirements
    to.object->remainingMaterials().remove(materialType, qtyToTake);
    for (const User *otherUser : findUsersInArea(user.location()))
      if (to.object->permissions.doesUserHaveAccess(otherUser->name()))
        sendConstructionMaterialsMessage(*otherUser, *to.object);

    // Remove items from user
    slotFrom.second -= qtyToTake;
    if (slotFrom.second == 0) fromItem = {};
    sendInventoryMessage(user, slot1, obj1);

    // Check if this action completed construction
    if (!to.object->isBeingBuilt()) {
      // Send to all nearby players, since object appearance will
      // change
      for (const User *otherUser : findUsersInArea(user.location()))
        sendConstructionMaterialsMessage(*otherUser, *to.object);
      for (const std::string &owner :
           to.object->permissions.ownerAsUsernames()) {
        auto pUser = getUserByName(owner);
        if (pUser)
          sendConstructionMaterialsMessage(pUser->socket(), *to.object);
      }

      // Trigger completing user's unlocks
      if (user.knowsConstruction(to.object->type()->id()))
        ProgressLock::triggerUnlocks(user, ProgressLock::CONSTRUCTION,
                                     to.object->type());

      // Update quest progress for completing user
      user.addQuestProgress(Quest::Objective::CONSTRUCT,
                            to.object->type()->id());
    }

    // Return an item to the user, if required.
    if (itemToReturn != nullptr) user.giveItem(itemToReturn);

    return;
  }

  auto &slotTo = (*to.container)[slot2];
  auto &toItem = slotTo.first;

  if (from.object && from.object->classTag() == 'n' && toItem.hasItem() ||
      to.object && to.object->classTag() == 'n' && fromItem.hasItem())
    RETURN_WITH(ERROR_NPC_SWAP)

  // Check gear-slot compatibility
  if (obj1.isGear() && toItem.hasItem() && toItem.type()->gearSlot() != slot1)
    RETURN_WITH(ERROR_NOT_GEAR)
  if (obj2.isGear() && fromItem.hasItem() &&
      fromItem.type()->gearSlot() != slot2)
    RETURN_WITH(ERROR_NOT_GEAR)

  // Check gear requirements
  if (fromItem.hasItem() && obj2.isGear() && !user.canEquip(*fromItem.type()))
    return;
  if (toItem.hasItem() && obj1.isGear() && !user.canEquip(*toItem.type()))
    return;

  // Check whether soulbound items can be moved
  if (fromItem.isSoulbound()) {
    if (obj2.isEntity() && !to.object->permissions.isOwnedByPlayer(user.name()))
      RETURN_WITH(WARNING_OBJECT_MUST_BE_PRIVATE);
  }
  if (toItem.isSoulbound()) {
    if (obj1.isEntity() &&
        !from.object->permissions.isOwnedByPlayer(user.name()))
      RETURN_WITH(WARNING_OBJECT_MUST_BE_PRIVATE);
  }

  // Combine stack, if identical types
  auto shouldPerformNormalSwap = true;
  do {
    if (!(fromItem.hasItem() && toItem.hasItem())) break;
    auto identicalItems = fromItem.type() == toItem.type();
    if (!identicalItems) break;
    auto roomInDest = toItem.type()->stackSize() - slotTo.second;
    if (roomInDest == 0) break;

    auto qtyToMove = min(roomInDest, slotFrom.second);
    slotFrom.second -= qtyToMove;
    slotTo.second += qtyToMove;
    if (slotFrom.second == 0) fromItem = {};
    shouldPerformNormalSwap = false;

  } while (false);

  if (obj2.isGear())
    fromItem.onEquip();
  else if (obj1.isGear())
    toItem.onEquip();

  if (shouldPerformNormalSwap) {
    // Perform the swap
    ServerItem::Instance::swap(slotFrom, slotTo);

    // If gear was changed
    if (obj1.isGear() || obj2.isGear()) {
      // Update this player's stats
      user.updateStats();

      // Alert nearby users of the new equipment
      // Assumption: gear can only match a single gear slot.
      std::string gearID = "";
      auto gearSlot = size_t{};
      auto itemHealth = Hitpoints{};
      if (obj1.isGear()) {
        gearSlot = slot1;
        if (fromItem.hasItem()) {
          gearID = fromItem.type()->id();
          itemHealth = fromItem.health();
        }
      } else {
        gearSlot = slot2;
        if (toItem.hasItem()) {
          gearID = toItem.type()->id();
          itemHealth = toItem.health();
        }
      }
      for (const User *otherUser : findUsersInArea(user.location())) {
        if (otherUser == &user) continue;
        sendMessage(
            otherUser->socket(),
            {SV_GEAR, makeArgs(user.name(), gearSlot, gearID, itemHealth)});
      }
    }
  }

  // Alert relevant users
  if (obj1.isInventory() || obj1.isGear())
    sendInventoryMessage(user, slot1, obj1);
  else
    from.object->tellRelevantUsersAboutInventorySlot(slot1);

  if (obj2.isInventory() || obj2.isGear()) {
    sendInventoryMessage(user, slot2, obj2);
    ProgressLock::triggerUnlocks(user, ProgressLock::ITEM, toItem.type());
  } else
    to.object->tellRelevantUsersAboutInventorySlot(slot2);
}

HANDLE_MESSAGE(CL_CEDE) {
  auto serial = Serial{};
  READ_ARGS(serial);

  if (serial.isInventory() || serial.isGear()) RETURN_WITH(WARNING_DOESNT_EXIST)
  auto *ent = _entities.find(serial);

  if (!ent->permissions.isOwnedByPlayer(user.name()))
    RETURN_WITH(WARNING_NO_PERMISSION)

  const City::Name &city = _cities.getPlayerCity(user.name());
  if (city.empty()) RETURN_WITH(ERROR_NOT_IN_CITY)

  const auto *obj = dynamic_cast<const Object *>(ent);
  if (obj && obj->objType().isPlayerUnique()) RETURN_WITH(ERROR_CANNOT_CEDE)

  ent->permissions.setCityOwner(city);
}

HANDLE_MESSAGE(CL_CAST) {
  auto spellID = ""s;
  READ_ARGS(spellID);

  if (user.isStunned()) RETURN_WITH(WARNING_STUNNED)

  if (!user.getClass().knowsSpell(spellID)) RETURN_WITH(ERROR_DONT_KNOW_SPELL)
  if (user.isSpellCoolingDown(spellID)) return;

  auto *spell = findSpell(spellID);
  if (!spell) return;

  if (user.energy() < spell->cost()) RETURN_WITH(WARNING_NOT_ENOUGH_ENERGY)

  user.castSpell(*spell);
}

HANDLE_MESSAGE(CL_CAST_ITEM) {
  auto slot = size_t{};
  READ_ARGS(slot);

  if (user.isStunned()) RETURN_WITH(WARNING_STUNNED)
  if (slot >= User::INVENTORY_SIZE) RETURN_WITH(ERROR_INVALID_SLOT)
  auto &invSlot = user.inventory(slot);
  if (!invSlot.first.hasItem()) RETURN_WITH(ERROR_EMPTY_SLOT)
  const ServerItem &item = *invSlot.first.type();
  if (!item.castsSpellOnUse()) RETURN_WITH(ERROR_CANNOT_CAST_ITEM)
  if (invSlot.first.isBroken()) RETURN_WITH(WARNING_BROKEN_ITEM)

  if (item.returnsOnCast()) {
    auto toBeRemoved = ItemSet{};
    toBeRemoved.add(&item);
    auto toBeAdded = ItemSet{};
    toBeAdded.add(item.returnsOnCast(), 1);
    auto roomForReturnedItem =
        user.hasRoomToRemoveThenAdd(toBeRemoved, toBeAdded);
    if (!roomForReturnedItem) RETURN_WITH(WARNING_INVENTORY_FULL)
  }

  auto *spell = findSpell(item.spellToCastOnUse());
  if (!spell) return;

  user.cancelAction();
  auto result = user.castSpell(*spell, item.spellArg());
  if (result == FAIL) return;

  --invSlot.second;
  if (invSlot.second == 0) invSlot.first = {};
  sendInventoryMessage(user, slot, Serial::Inventory());

  if (item.returnsOnCast()) user.giveItem(item.returnsOnCast());
}

HANDLE_MESSAGE(CL_REPAIR_ITEM) {
  auto serial = Serial{};
  auto slot = size_t{};
  READ_ARGS(serial, slot);

  ServerItem::Instance *itemToRepair = nullptr;
  if (serial.isInventory())
    itemToRepair = &user.inventory(slot).first;
  else if (serial.isGear())
    itemToRepair = &user.gear(slot).first;
  else {  // Container object
    auto *ent = _entities.find(serial);
    auto *obj = dynamic_cast<Object *>(ent);

    if (!obj) RETURN_WITH(WARNING_DOESNT_EXIST)

    if (!obj->permissions.doesUserHaveAccess(user.name()))
      RETURN_WITH(WARNING_NO_PERMISSION)

    auto numSlots = obj->objType().container().slots();
    if (slot >= numSlots) RETURN_WITH(ERROR_INVALID_SLOT)

    itemToRepair = &obj->container().at(slot).first;
  }

  const auto &repairInfo = itemToRepair->type()->repairInfo();
  if (!repairInfo.canBeRepaired) RETURN_WITH(WARNING_NOT_REPAIRABLE)

  auto costItem = findItem(repairInfo.cost);
  if (repairInfo.hasCost()) {
    auto cost = ItemSet{};
    cost.add(costItem);

    if (!user.hasItems(cost)) RETURN_WITH(WARNING_ITEM_NEEDED)
  }

  if (repairInfo.requiresTool()) {
    if (!user.checkAndDamageToolAndGetSpeed(repairInfo.tool)) {
      user.sendMessage({WARNING_ITEM_TAG_NEEDED, repairInfo.tool});
      return;
    }
  }

  if (repairInfo.hasCost()) {
    auto itemToRemove = ItemSet{};
    itemToRemove.add(costItem);
    user.removeItems(itemToRemove);
  }

  auto wasBroken = itemToRepair->isBroken();

  itemToRepair->repair();

  if (wasBroken) user.updateStats();
}

HANDLE_MESSAGE(CL_TAME_NPC) {
  auto serial = Serial{};
  READ_ARGS(serial);

  auto *npc = _entities.find<NPC>(serial);
  if (!npc) RETURN_WITH(WARNING_DOESNT_EXIST)

  const auto &type = *npc->npcType();
  if (!type.canBeTamed()) return;
  if (npc->permissions.hasOwner()) return;

  auto consumable = ItemSet{};
  if (!type.tamingRequiresItem().empty()) {
    const auto *item = findItem(type.tamingRequiresItem());
    consumable.add(item);
    if (!user.hasItems(consumable)) RETURN_WITH(WARNING_ITEM_NEEDED)
  }

  // All checks pass; attempt goes ahead.

  user.removeItems(consumable);

  if (randDouble() > npc->getTameChance()) {
    user.sendMessage(SV_TAME_ATTEMPT_FAILED);
    return;
  }

  npc->includeInPersistentState();
  npc->separateFromSpawner();
  npc->permissions.setPlayerOwner(user.name());

  if (user.target() == npc) {
    user.finishAction();
  }

  if (user.hasRoomForMoreFollowers())
    user.followers.add();
  else
    npc->order(NPC::STAY);
}

HANDLE_MESSAGE(CL_FEED_PET) {
  auto serial = Serial{};
  READ_ARGS(serial);

  auto *npc = _entities.find<NPC>(serial);
  if (!npc) RETURN_WITH(WARNING_DOESNT_EXIST)
  const auto it = _buffTypes.find("food");
  if (it == _buffTypes.end()) return;
  if (!npc->permissions.isOwnedByPlayer(user.name()))
    RETURN_WITH(WARNING_NO_PERMISSION)
  if (distance(npc->location(), user.location()) > ACTION_DISTANCE)
    RETURN_WITH(WARNING_TOO_FAR)
  if (npc->health() == npc->stats().maxHealth)
    RETURN_WITH(WARNING_PET_AT_FULL_HEALTH)
  if (!user.hasItems("food", 1)) RETURN_WITH(WARNING_ITEM_NEEDED)

  npc->applyBuff(it->second, user);
  user.removeItems("food", 1);
}

HANDLE_MESSAGE(CL_ORDER_NPC_TO_STAY) {
  auto serial = Serial{};
  READ_ARGS(serial);

  auto *npc = _entities.find<NPC>(serial);
  if (!npc) RETURN_WITH(WARNING_DOESNT_EXIST)
  if (!npc->permissions.isOwnedByPlayer(user.name()))
    RETURN_WITH(WARNING_NO_PERMISSION)
  if (distance(npc->location(), user.location()) > ACTION_DISTANCE)
    RETURN_WITH(WARNING_TOO_FAR)
  if (npc->order() == NPC::STAY) RETURN_WITH(WARNING_PET_IS_ALREADY_STAYING)

  user.followers.remove();
  npc->order(NPC::STAY);
}

HANDLE_MESSAGE(CL_ORDER_NPC_TO_FOLLOW) {
  auto serial = Serial{};
  READ_ARGS(serial);

  auto *npc = _entities.find<NPC>(serial);
  if (!npc) RETURN_WITH(WARNING_DOESNT_EXIST)
  if (!npc->permissions.isOwnedByPlayer(user.name()))
    RETURN_WITH(WARNING_NO_PERMISSION)
  if (distance(npc->location(), user.location()) > ACTION_DISTANCE)
    RETURN_WITH(WARNING_TOO_FAR)
  if (npc->order() == NPC::FOLLOW) RETURN_WITH(WARNING_PET_IS_ALREADY_FOLLOWING)
  if (!user.hasRoomForMoreFollowers())
    RETURN_WITH(WARNING_NO_ROOM_FOR_MORE_FOLLOWERS)

  user.followers.add();
  npc->order(NPC::FOLLOW);
}

HANDLE_MESSAGE(CL_INVITE_TO_GROUP) {
  auto inviteeName = ""s;
  READ_ARGS(inviteeName);
  inviteeName = toPascal(inviteeName);

  if (inviteeName == user.name()) return;
  auto *invitee = getUserByName(inviteeName);
  if (!invitee) RETURN_WITH(ERROR_USER_NOT_FOUND);
  if (groups->isUserInAGroup(inviteeName))
    RETURN_WITH(WARNING_USER_ALREADY_IN_A_GROUP);

  groups->registerInvitation(user.name(), inviteeName);
  invitee->sendMessage({SV_INVITED_TO_GROUP, user.name()});
}

HANDLE_MESSAGE(CL_ACCEPT_GROUP_INVITATION) {
  if (groups->isUserInAGroup(user.name())) return;
  if (groups->userHasAnInvitation(user.name()))
    groups->acceptInvitation(user.name());
}

HANDLE_MESSAGE(CL_LEAVE_GROUP) { groups->removeUserFromHisGroup(user.name()); }

HANDLE_MESSAGE(CL_ROLL) {
  auto result = roll();
  broadcastToGroup(user.name(),
                   {SV_ROLL_RESULT, makeArgs(user.name(), result)});
}

HANDLE_MESSAGE(DG_UNLOCK) {
  CHECK_NO_ARGS;

  if (!isDebug()) return;
  ProgressLock::unlockAll(user);
}

#define SEND_MESSAGE_TO_HANDLER(MESSAGE_CODE)           \
  case MESSAGE_CODE:                                    \
    handleMessage<MESSAGE_CODE>(client, *user, parser); \
    break;
// TODO: Remove
#define BREAK_WITH(MSG)       \
  {                           \
    sendMessage(client, MSG); \
    return;                   \
  }

void Server::handleBufferedMessages(const Socket &client,
                                    const std::string &messages) {
  _debug(messages);
  char del;
  MessageParser parser(messages);
  User *user = nullptr;
  while (parser.hasAnotherMessage()) {
    auto msgCode = parser.nextMessage();

    // Discard message if this client has not yet logged in
    auto it = _users.find(client);
    auto userHasLoggedIn = it != _users.end();
    if (!userHasLoggedIn && !isMessageAllowedBeforeLogin(msgCode)) {
      continue;
    }

    if (userHasLoggedIn) {
      User &userRef = const_cast<User &>(*it);
      user = &userRef;
      user->contact();
    }

    auto &iss = parser.iss;
    del = parser.getLastDelimiterRead();

    switch (msgCode) {
      SEND_MESSAGE_TO_HANDLER(CL_REPORT_BUG)
      SEND_MESSAGE_TO_HANDLER(CL_PING)
      SEND_MESSAGE_TO_HANDLER(CL_LOGIN_EXISTING)
      SEND_MESSAGE_TO_HANDLER(CL_LOGIN_NEW)
      SEND_MESSAGE_TO_HANDLER(CL_REQUEST_TIME_PLAYED)
      SEND_MESSAGE_TO_HANDLER(CL_SKIP_TUTORIAL)
      SEND_MESSAGE_TO_HANDLER(CL_LOCATION)
      SEND_MESSAGE_TO_HANDLER(CL_CRAFT)
      SEND_MESSAGE_TO_HANDLER(CL_CONSTRUCT)
      SEND_MESSAGE_TO_HANDLER(CL_CONSTRUCT_FOR_CITY)
      SEND_MESSAGE_TO_HANDLER(CL_TRADE)
      SEND_MESSAGE_TO_HANDLER(CL_DROP)
      SEND_MESSAGE_TO_HANDLER(CL_PICK_UP_DROPPED_ITEM)
      SEND_MESSAGE_TO_HANDLER(CL_SWAP_ITEMS)
      SEND_MESSAGE_TO_HANDLER(CL_CEDE)
      SEND_MESSAGE_TO_HANDLER(CL_CAST)
      SEND_MESSAGE_TO_HANDLER(CL_CAST_ITEM)
      SEND_MESSAGE_TO_HANDLER(CL_REPAIR_ITEM)
      SEND_MESSAGE_TO_HANDLER(CL_TAME_NPC)
      SEND_MESSAGE_TO_HANDLER(CL_FEED_PET)
      SEND_MESSAGE_TO_HANDLER(CL_ORDER_NPC_TO_STAY)
      SEND_MESSAGE_TO_HANDLER(CL_ORDER_NPC_TO_FOLLOW)
      SEND_MESSAGE_TO_HANDLER(CL_INVITE_TO_GROUP)
      SEND_MESSAGE_TO_HANDLER(CL_ACCEPT_GROUP_INVITATION)
      SEND_MESSAGE_TO_HANDLER(CL_LEAVE_GROUP)
      SEND_MESSAGE_TO_HANDLER(CL_ROLL)
      SEND_MESSAGE_TO_HANDLER(DG_UNLOCK)

      case CL_CONSTRUCT_FROM_ITEM:
      case CL_CONSTRUCT_FROM_ITEM_FOR_CITY: {
        size_t slot;
        double x, y;
        iss >> slot >> del >> x >> del >> y >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) BREAK_WITH(WARNING_STUNNED)
        user->cancelAction();
        if (slot >= User::INVENTORY_SIZE) BREAK_WITH(ERROR_INVALID_SLOT)
        const auto &invSlot = user->inventory(slot);
        if (!invSlot.first.hasItem()) BREAK_WITH(ERROR_EMPTY_SLOT)
        const ServerItem &item = *invSlot.first.type();
        if (item.constructsObject() == nullptr)
          BREAK_WITH(ERROR_CANNOT_CONSTRUCT);
        if (invSlot.first.isBroken()) BREAK_WITH(WARNING_BROKEN_ITEM)
        const MapPoint location(x, y);
        const ObjectType &objType = *item.constructsObject();
        if (distance(user->collisionRect(),
                     objType.collisionRect() + location) > ACTION_DISTANCE)
          BREAK_WITH(WARNING_TOO_FAR)
        if (!isLocationValid(location, objType)) BREAK_WITH(WARNING_BLOCKED)

        // Tool check must be the last check, as it damages the tools.
        auto requiresTool = !objType.constructionReq().empty();
        auto toolSpeed = 1.0;
        if (requiresTool) {
          toolSpeed =
              user->checkAndDamageToolAndGetSpeed(objType.constructionReq());
          if (toolSpeed == 0) BREAK_WITH(WARNING_NEED_TOOLS)
        }

        auto ownerIsCity = msgCode == CL_CONSTRUCT_FROM_ITEM_FOR_CITY;
        user->beginConstructing(objType, location, ownerIsCity, toolSpeed,
                                slot);
        sendMessage(client, {SV_ACTION_STARTED,
                             objType.constructionTime() / toolSpeed});
        break;
      }

      case CL_DISMISS_BUFF: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        auto buffID = std::string{_stringInputBuffer};
        iss >> del;
        if (del != MSG_END) return;

        if (user->isStunned()) BREAK_WITH(WARNING_STUNNED)

        user->removeBuff(buffID);
        break;
      }

      case CL_CANCEL_ACTION: {
        iss >> del;
        if (del != MSG_END) return;
        user->cancelAction();
        break;
      }

      case CL_GATHER: {
        auto serial = Serial{};
        iss >> serial >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) BREAK_WITH(WARNING_STUNNED)
        user->cancelAction();
        auto *ent = _entities.find(serial);
        if (!isEntityInRange(client, *user, ent)) {
          // isEntityInRange() sends error messages if applicable.
          break;
        }
        if (!ent->type()) {
          SERVER_ERROR("Can't gather from object with no type");
          break;
        }

        auto *asObject = dynamic_cast<Object *>(ent);
        if (asObject) {
          // Check that the user meets the requirements
          const auto &exclusiveQuestID = asObject->objType().exclusiveToQuest();
          auto isQuestExclusive = !exclusiveQuestID.empty();
          if (isQuestExclusive) {
            auto userIsOnQuest =
                user->questsInProgress().count(exclusiveQuestID) == 1;
            if (!userIsOnQuest) break;
          }
          if (asObject->isBeingBuilt()) BREAK_WITH(ERROR_UNDER_CONSTRUCTION)
          // Check whether it has an inventory
          if (asObject->hasContainer() && !asObject->container().isEmpty())
            BREAK_WITH(WARNING_NOT_EMPTY)
        }
        if (!ent->permissions.doesUserHaveAccess(user->name()))
          BREAK_WITH(WARNING_NO_PERMISSION)

        // Tool check must be the last check, as it damages the tools.
        const auto &gatherReq = ent->type()->yield.requiredTool();
        auto toolSpeed = 1.0;
        auto requiresTool = !gatherReq.empty();
        if (requiresTool) {
          toolSpeed = user->checkAndDamageToolAndGetSpeed(gatherReq);
          if (toolSpeed == 0) {
            sendMessage(client, {WARNING_ITEM_TAG_NEEDED, gatherReq});
            break;
          }
        }

        user->beginGathering(ent, toolSpeed);

        break;
      }

      case CL_DECONSTRUCT: {
        Serial serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        user->cancelAction();
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) BREAK_WITH(WARNING_TOO_FAR)
        if (obj->isBeingBuilt()) BREAK_WITH(ERROR_UNDER_CONSTRUCTION)

        if (!obj->type()) {
          SERVER_ERROR("Can't deconstruct object with no type");
          break;
        }

        if (!obj->permissions.doesUserHaveAccess(user->name()))
          BREAK_WITH(WARNING_NO_PERMISSION)
        // Check that the object can be deconstructed
        if (!obj->hasDeconstruction()) BREAK_WITH(ERROR_CANNOT_DECONSTRUCT)
        if (obj->health() < obj->stats().maxHealth)
          BREAK_WITH(ERROR_DAMAGED_OBJECT)
        if (!obj->isAbleToDeconstruct(*user)) {
          break;
        }
        // Check that it isn't an occupied vehicle
        if (obj->classTag() == 'v' &&
            !dynamic_cast<const Vehicle *>(obj)->driver().empty())
          BREAK_WITH(WARNING_VEHICLE_OCCUPIED)

        user->beginDeconstructing(*obj);
        sendMessage(client, {SV_ACTION_STARTED,
                             obj->deconstruction().timeToDeconstruct()});
        break;
      }

      case CL_DEMOLISH: {
        Serial serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) BREAK_WITH(WARNING_STUNNED)
        user->cancelAction();
        auto *ent = _entities.find(serial);
        if (!isEntityInRange(client, *user, ent)) BREAK_WITH(WARNING_TOO_FAR)
        if (!ent->type()) {
          SERVER_ERROR("Can't demolish object with no type");
          break;
        }

        auto userHasPermission = bool{};
        if (ent->classTag() == 'n') {
          auto *npc = dynamic_cast<NPC *>(ent);
          userHasPermission = npc->permissions.canUserDemolish(user->name());
        } else {
          const auto *obj = dynamic_cast<Object *>(ent);
          userHasPermission = obj->permissions.canUserDemolish(user->name());
        }
        if (!userHasPermission) BREAK_WITH(WARNING_NO_PERMISSION)

        // Check that it isn't an occupied vehicle
        if (ent->classTag() == 'v' &&
            !dynamic_cast<const Vehicle *>(ent)->driver().empty())
          BREAK_WITH(WARNING_VEHICLE_OCCUPIED)

        ent->kill();
        ent->setShorterCorpseTimerForFriendlyKill();
        break;
      }

      case CL_AUTO_CONSTRUCT: {
        Serial serial;
        iss >> serial >> del;
        if (del != MSG_END) return;

        handle_CL_AUTO_CONSTRUCT(*user, serial);
        break;
      }

      case CL_TAKE_ITEM: {
        Serial serial;
        size_t slotNum;
        iss >> serial >> del >> slotNum >> del;
        if (del != MSG_END) return;

        if (user->isStunned()) BREAK_WITH(WARNING_STUNNED)

        handle_CL_TAKE_ITEM(*user, serial, slotNum);
        break;
      }

      case CL_SET_MERCHANT_SLOT: {
        Serial serial;
        size_t slot, wareQty, priceQty;
        iss >> serial >> del >> slot >> del;
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_DELIM);
        std::string ware(_stringInputBuffer);
        iss >> del >> wareQty >> del;
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_DELIM);
        std::string price(_stringInputBuffer);
        iss >> del >> priceQty >> del;
        if (del != MSG_END) return;
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) break;
        if (obj->isBeingBuilt()) BREAK_WITH(ERROR_UNDER_CONSTRUCTION)
        if (!obj->permissions.doesUserHaveAccess(user->name()))
          BREAK_WITH(WARNING_NO_PERMISSION)
        size_t slots = obj->objType().merchantSlots();
        if (slots == 0) BREAK_WITH(ERROR_NOT_MERCHANT)
        if (slot >= slots) BREAK_WITH(ERROR_INVALID_MERCHANT_SLOT)
        auto wareIt = _items.find(ware);
        if (wareIt == _items.end()) BREAK_WITH(ERROR_INVALID_ITEM)
        auto priceIt = _items.find(price);
        if (priceIt == _items.end()) BREAK_WITH(ERROR_INVALID_ITEM)
        MerchantSlot &mSlot = obj->merchantSlot(slot);
        mSlot = MerchantSlot(&*wareIt, wareQty, &*priceIt, priceQty);

        // Alert watchers
        obj->tellRelevantUsersAboutMerchantSlot(slot);
        break;
      }

      case CL_CLEAR_MERCHANT_SLOT: {
        Serial serial;
        size_t slot;
        iss >> serial >> del >> slot >> del;
        if (del != MSG_END) return;
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) break;
        if (obj->isBeingBuilt()) BREAK_WITH(ERROR_UNDER_CONSTRUCTION)
        if (!obj->permissions.doesUserHaveAccess(user->name()))
          BREAK_WITH(WARNING_NO_PERMISSION)
        size_t slots = obj->objType().merchantSlots();
        if (slots == 0) BREAK_WITH(ERROR_NOT_MERCHANT)
        if (slot >= slots) BREAK_WITH(ERROR_INVALID_MERCHANT_SLOT)
        obj->merchantSlot(slot) = MerchantSlot();

        // Alert watchers
        obj->tellRelevantUsersAboutMerchantSlot(slot);
        break;
      }

      case CL_REPAIR_OBJECT: {
        Serial serial;
        iss >> serial >> del;
        if (del != MSG_END) return;

        handle_CL_REPAIR_OBJECT(*user, serial);
        break;
      }

      case CL_MOUNT: {
        Serial serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) BREAK_WITH(WARNING_STUNNED)
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) break;
        if (obj->isBeingBuilt()) BREAK_WITH(ERROR_UNDER_CONSTRUCTION)
        if (!obj->permissions.doesUserHaveAccess(user->name()))
          BREAK_WITH(WARNING_NO_PERMISSION)
        if (obj->classTag() != 'v') BREAK_WITH(ERROR_NOT_VEHICLE)
        Vehicle *v = dynamic_cast<Vehicle *>(obj);
        if (!v->driver().empty() && v->driver() != user->name())
          BREAK_WITH(WARNING_VEHICLE_OCCUPIED)

        v->driver(user->name());
        user->driving(v->serial());
        user->teleportTo(v->location());
        // Alert nearby users (including the new driver)
        for (const User *u : findUsersInArea(user->location()))
          sendMessage(u->socket(),
                      {SV_MOUNTED, makeArgs(serial, user->name())});

        user->onTerrainListChange(v->allowedTerrain().id());

        break;
      }

      case CL_DISMOUNT: {
        if (del != MSG_END) return;
        if (user->isStunned()) BREAK_WITH(WARNING_STUNNED)
        if (!user->isDriving()) BREAK_WITH(ERROR_NOT_VEHICLE)

        auto dst = MapPoint{};
        bool validDestinationFound = false;
        const auto ATTEMPTS = 50;
        for (auto i = 0; i != ATTEMPTS; ++i) {
          dst = getRandomPointInCircle(user->location(), ACTION_DISTANCE);
          if (isLocationValid(dst, User::OBJECT_TYPE)) {
            validDestinationFound = true;
            break;
          }
        }

        if (!validDestinationFound)
          BREAK_WITH(WARNING_NO_VALID_DISMOUNT_LOCATION)

        auto serial = user->driving();
        Object *obj = _entities.find<Object>(serial);
        Vehicle *v = dynamic_cast<Vehicle *>(obj);

        v->driver("");
        user->driving({});
        for (const User *u : findUsersInArea(user->location()))
          sendMessage(u->socket(),
                      {SV_UNMOUNTED, makeArgs(serial, user->name())});

        // Teleport him him, to avoid collision with the vehicle.
        user->teleportTo(dst);

        user->onTerrainListChange(TerrainList::defaultList().id());

        break;
      }

      case CL_TARGET_ENTITY: {
        Serial serial;
        iss >> serial >> del;
        if (del != MSG_END) return;

        handle_CL_TARGET_ENTITY(*user, serial);
        break;
      }

      case CL_TARGET_PLAYER: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        std::string targetUsername(_stringInputBuffer);
        iss >> del;
        if (del != MSG_END) return;

        handle_CL_TARGET_PLAYER(*user, targetUsername);
        break;
      }

      case CL_SELECT_ENTITY: {
        Serial serial;
        iss >> serial >> del;
        if (del != MSG_END) return;

        handle_CL_SELECT_ENTITY(*user, serial);
        break;
      }

      case CL_SELECT_PLAYER: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        std::string targetUsername(_stringInputBuffer);
        iss >> del;
        if (del != MSG_END) return;

        handle_CL_SELECT_PLAYER(*user, targetUsername);
        break;
      }

      case CL_DECLARE_WAR_ON_PLAYER:
      case CL_DECLARE_WAR_ON_CITY: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        std::string targetName(_stringInputBuffer);
        iss >> del;
        if (del != MSG_END) return;

        targetName = toPascal(targetName);
        auto thisBelligerent = Belligerent{user->name(), Belligerent::PLAYER};
        Belligerent::Type targetType = msgCode == CL_DECLARE_WAR_ON_PLAYER
                                           ? Belligerent::PLAYER
                                           : Belligerent::CITY;
        auto targetBelligerent = Belligerent{targetName, targetType};

        if (_wars.isAtWarExact(user->name(), targetBelligerent))
          BREAK_WITH(ERROR_ALREADY_AT_WAR)

        _wars.declare(thisBelligerent, targetBelligerent);
        break;
      }

      case CL_DECLARE_WAR_ON_PLAYER_AS_CITY:
      case CL_DECLARE_WAR_ON_CITY_AS_CITY: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        std::string targetName(_stringInputBuffer);
        iss >> del;
        if (del != MSG_END) return;

        if (!_cities.isPlayerInACity(user->name()))
          BREAK_WITH(ERROR_NOT_IN_CITY)
        if (!_kings.isPlayerAKing(user->name())) BREAK_WITH(ERROR_NOT_A_KING)
        auto declarer =
            Belligerent{_cities.getPlayerCity(user->name()), Belligerent::CITY};

        targetName = toPascal(targetName);
        Belligerent::Type targetType =
            msgCode == CL_DECLARE_WAR_ON_PLAYER_AS_CITY ? Belligerent::PLAYER
                                                        : Belligerent::CITY;
        auto targetBelligerent = Belligerent{targetName, targetType};

        if (_wars.isAtWarExact(declarer, targetBelligerent))
          BREAK_WITH(ERROR_ALREADY_AT_WAR)

        _wars.declare(declarer, targetBelligerent);
        break;
      }

      case CL_SUE_FOR_PEACE_WITH_PLAYER:
      case CL_SUE_FOR_PEACE_WITH_CITY:
      case CL_SUE_FOR_PEACE_WITH_PLAYER_AS_CITY:
      case CL_SUE_FOR_PEACE_WITH_CITY_AS_CITY: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        auto name = std::string{_stringInputBuffer};
        iss >> del;
        if (del != MSG_END) return;
        handle_CL_SUE_FOR_PEACE(*user, static_cast<MessageCode>(msgCode), name);
        break;
      }

      case CL_CANCEL_PEACE_OFFER_TO_PLAYER:
      case CL_CANCEL_PEACE_OFFER_TO_CITY:
      case CL_CANCEL_PEACE_OFFER_TO_PLAYER_AS_CITY:
      case CL_CANCEL_PEACE_OFFER_TO_CITY_AS_CITY: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        auto name = std::string{_stringInputBuffer};
        iss >> del;
        if (del != MSG_END) return;
        handle_CL_CANCEL_PEACE_OFFER(*user, static_cast<MessageCode>(msgCode),
                                     name);
        break;
      }

      case CL_ACCEPT_PEACE_OFFER_WITH_PLAYER:
      case CL_ACCEPT_PEACE_OFFER_WITH_CITY:
      case CL_ACCEPT_PEACE_OFFER_WITH_PLAYER_AS_CITY:
      case CL_ACCEPT_PEACE_OFFER_WITH_CITY_AS_CITY: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        auto name = std::string{_stringInputBuffer};
        iss >> del;
        if (del != MSG_END) return;
        handle_CL_ACCEPT_PEACE_OFFER(*user, static_cast<MessageCode>(msgCode),
                                     name);
        break;
      }

      case CL_LEAVE_CITY: {
        if (del != MSG_END) return;
        handle_CL_LEAVE_CITY(*user);
        break;
      }

      case CL_GRANT: {
        auto serial = Serial{};
        iss >> serial >> del;
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        auto username = std::string{_stringInputBuffer};
        iss >> del;
        if (del != MSG_END) return;
        handle_CL_GRANT(*user, serial, username);
        break;
      }

      case CL_PERFORM_OBJECT_ACTION: {
        auto serial = Serial{};
        iss >> serial >> del;
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        auto textArg = std::string{_stringInputBuffer};
        iss >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) BREAK_WITH(WARNING_STUNNED)
        handle_CL_PERFORM_OBJECT_ACTION(*user, serial, textArg);

        break;
      }

      case CL_RECRUIT: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        auto username = std::string{_stringInputBuffer};
        iss >> del;
        if (del != MSG_END) return;
        handle_CL_RECRUIT(*user, username);
        break;
      }

      case CL_TAKE_TALENT: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        auto talentName = Talent::Name{_stringInputBuffer};
        iss >> del;
        if (del != MSG_END) return;
        handle_CL_TAKE_TALENT(*user, talentName);
        break;
      }

      case CL_UNLEARN_TALENTS: {
        if (del != MSG_END) return;
        handle_CL_UNLEARN_TALENTS(*user);
        break;
      }

      case CL_ACCEPT_QUEST: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_DELIM);
        auto questID = Quest::ID{_stringInputBuffer};
        iss >> del;

        auto startSerial = Serial{};
        iss >> startSerial >> del;

        if (del != MSG_END) return;

        handle_CL_ACCEPT_QUEST(*user, questID, startSerial);
        break;
      }

      case CL_COMPLETE_QUEST: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_DELIM);
        auto questID = Quest::ID{_stringInputBuffer};
        iss >> del;

        auto endSerial = Serial{};
        iss >> endSerial >> del;

        if (del != MSG_END) return;

        handle_CL_COMPLETE_QUEST(*user, questID, endSerial);
        break;
      }

      case CL_ABANDON_QUEST: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        auto questID = Quest::ID{_stringInputBuffer};
        iss >> del;

        if (del != MSG_END) return;

        handle_CL_ABANDON_QUEST(*user, questID);
        break;
      }

      case CL_HOTBAR_BUTTON: {
        auto slot = 0, category = 0;
        auto id = ""s;
        iss >> slot >> del >> category >> del;
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        id = _stringInputBuffer;
        iss >> del;
        user->setHotbarAction(slot, category, id);
        break;
      }

      case CL_ACKNOWLEDGE_DEATH:
        user->isWaitingForDeathAcknowledgement = false;
        break;

      case CL_SAY: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        std::string message(_stringInputBuffer);
        iss >> del;
        if (del != MSG_END) return;
        broadcast({SV_SAY, makeArgs(user->name(), message)});

        auto fs = std::ofstream{"chat.log", std::ios_base::app};
        fs << user->name() << ": " << message << std::endl;

        break;
      }

      case CL_WHISPER: {
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_DELIM);
        std::string username(_stringInputBuffer);
        iss >> del;
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        std::string message(_stringInputBuffer);
        iss >> del;
        if (del != MSG_END) return;
        auto it = _usersByName.find(username);
        if (it == _usersByName.end()) BREAK_WITH(ERROR_INVALID_USER)
        const User *target = it->second;
        sendMessage(target->socket(),
                    {SV_WHISPER, makeArgs(user->name(), message)});

        auto fs = std::ofstream{"chat.log", std::ios_base::app};
        fs << user->name() << ">" << username << ": " << message << std::endl;

        break;
      }

      case DG_GIVE: {
        if (!isDebug()) break;
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        std::string id(_stringInputBuffer);
        iss >> del;
        if (del != MSG_END) return;
        const auto it = _items.find(id);
        if (it == _items.end()) BREAK_WITH(ERROR_INVALID_ITEM)
        const ServerItem &item = *it;
        ;
        user->giveItem(&item, item.stackSize());
        break;
      }

      case DG_GIVE_OBJECT: {
        if (!isDebug()) break;
        iss.get(_stringInputBuffer, BUFFER_SIZE, MSG_END);
        std::string id(_stringInputBuffer);
        iss >> del;
        if (del != MSG_END) return;
        auto *ot = findObjectTypeByID(id);
        if (!ot) BREAK_WITH(ERROR_INVALID_OBJECT)
        if (ot->classTag() == 'n') {
          auto *nt = dynamic_cast<NPCType *>(ot);
          if (!nt) break;
          auto &npc = addNPC(nt, user->location() + MapPoint{50, 0});
          npc.permissions.setPlayerOwner(user->name());
        } else {
          auto owner =
              Permissions::Owner{Permissions::Owner::PLAYER, user->name()};
          auto &obj = addObject(ot, user->location() + MapPoint{50, 0}, owner);
          if (obj.isBeingBuilt()) {
            obj.remainingMaterials().clear();
            sendConstructionMaterialsMessage(*user, obj);
          }
        }
        break;
      }

      case DG_LEVEL: {
        if (del != MSG_END) return;
        if (!isDebug()) break;
        auto remainingXP = User::XP_PER_LEVEL[user->level()] - user->xp();
        user->addXP(remainingXP);
        break;
      }

      case DG_TELEPORT: {
        double x, y;
        iss >> x >> del >> y >> del;
        if (del != MSG_END) return;
        if (!isDebug()) break;

        user->teleportTo({x, y});
        break;
      }

      case DG_SPELLS: {
        if (!isDebug()) break;
        if (del != MSG_END) return;
        for (auto &pair : _spells) {
          const auto &spell = *pair.second;
          user->getClass().teachSpell(spell.id());
        }
        auto knownSpellsString = user->getClass().generateKnownSpellsString();
        user->sendMessage({SV_KNOWN_SPELLS, knownSpellsString});
        break;
      }

      case DG_DIE: {
        if (del != MSG_END) return;

        user->kill();
        break;
      }

      default:
        _debug << Color::CHAT_ERROR << "Unhandled message: " << messages
               << Log::endl;
    }
  }
}

// TODO: remove
#undef RETURN_WITH
#define RETURN_WITH(MSG)   \
  {                        \
    user.sendMessage(MSG); \
    return;                \
  }

void Server::handle_CL_TAKE_ITEM(User &user, Serial serial, size_t slotNum) {
  if (serial.isInventory()) RETURN_WITH(ERROR_TAKE_SELF)

  Entity *pEnt = nullptr;
  ServerItem::Slot *pSlot;
  if (serial.isGear())
    pSlot = user.getSlotToTakeFromAndSendErrors(slotNum, user);
  else {
    pEnt = _entities.find(serial);
    if (!pEnt) RETURN_WITH(WARNING_DOESNT_EXIST)
    pSlot = pEnt->getSlotToTakeFromAndSendErrors(slotNum, user);
  }
  if (!pSlot) return;
  ServerItem::Slot &slot = *pSlot;

  auto userHasPermissionToLoot =
      groups->areUsersInSameGroup(user.name(), pEnt->tagger.username());
  if (!userHasPermissionToLoot) return;

  // Attempt to give item to user
  size_t remainder = user.giveItem(slot.first.type(), slot.second);
  if (remainder > 0) {
    slot.second = remainder;
    sendMessage(user.socket(), WARNING_INVENTORY_FULL);
  } else {
    slot.first = {};
    slot.second = 0;
  }

  if (serial.isGear()) {  // Tell user about his empty gear slot, and updated
                          // stats
    sendInventoryMessage(user, slotNum, serial);
    user.updateStats();

  } else if (pEnt->isDead())  // Loot?
    pEnt->tellRelevantUsersAboutLootSlot(slotNum);

  else {  // Container
    auto *asObject = dynamic_cast<const Object *>(pEnt);
    if (!asObject) {
      SERVER_ERROR("Don't know how to handle TAKE_ITEM request");
      return;
    }
    asObject->tellRelevantUsersAboutInventorySlot(slotNum);
  }
}

void Server::handle_CL_REPAIR_OBJECT(User &user, Serial serial) {
  auto it = _entities.find(serial);
  auto *obj = dynamic_cast<Object *>(&*it);

  if (!obj) RETURN_WITH(WARNING_DOESNT_EXIST)

  const auto &repairInfo = obj->objType().repairInfo();
  if (!repairInfo.canBeRepaired) RETURN_WITH(WARNING_NOT_REPAIRABLE)

  auto costItem = findItem(repairInfo.cost);
  if (repairInfo.hasCost()) {
    auto cost = ItemSet{};
    cost.add(costItem);

    if (!user.hasItems(cost)) RETURN_WITH(WARNING_ITEM_NEEDED)
  }

  if (repairInfo.requiresTool()) {
    if (!user.checkAndDamageToolAndGetSpeed(repairInfo.tool)) {
      user.sendMessage({WARNING_ITEM_TAG_NEEDED, repairInfo.tool});
      return;
    }
  }

  if (repairInfo.hasCost()) {
    auto itemToRemove = ItemSet{};
    itemToRemove.add(costItem);
    user.removeItems(itemToRemove);
  }

  obj->repair();
}

void Server::handle_CL_LEAVE_CITY(User &user) {
  auto city = _cities.getPlayerCity(user.name());
  if (city.empty()) RETURN_WITH(ERROR_NOT_IN_CITY)
  if (_kings.isPlayerAKing(user.name()))
    RETURN_WITH(ERROR_KING_CANNOT_LEAVE_CITY)
  _cities.removeUserFromCity(user, city);
}

void Server::handle_CL_GRANT(User &user, Serial serial, std::string username) {
  auto *obj = _entities.find<Object>(serial);
  if (!obj) RETURN_WITH(WARNING_DOESNT_EXIST)
  auto playerCity = cities().getPlayerCity(user.name());
  if (!obj->permissions.isOwnedByCity(playerCity))
    RETURN_WITH(WARNING_NO_PERMISSION)
  if (!_kings.isPlayerAKing(user.name())) RETURN_WITH(ERROR_NOT_A_KING)
  username = toPascal(username);
  if (!cities().isPlayerInCity(username, playerCity))
    RETURN_WITH(WARNING_NOT_A_CITIZEN)

  obj->permissions.setPlayerOwner(username);
}

void Server::handle_CL_PERFORM_OBJECT_ACTION(User &user, Serial serial,
                                             const std::string &textArg) {
  if (!serial.isEntity()) RETURN_WITH(WARNING_DOESNT_EXIST)
  auto *obj = _entities.find<Object>(serial);

  if (!obj->permissions.doesUserHaveAccess(user.name(), true))
    RETURN_WITH(WARNING_NO_PERMISSION)

  const auto &objType = obj->objType();
  if (!objType.hasAction()) RETURN_WITH(ERROR_NO_ACTION)

  if (objType.action().cost) {
    auto cost = ItemSet{};
    cost.add(objType.action().cost);
    if (!user.hasItems(cost)) RETURN_WITH(WARNING_ITEM_NEEDED)
  }

  auto succeeded = objType.action().function(*obj, user, textArg);

  if (succeeded) {
    if (objType.action().cost) {
      auto cost = ItemSet{};
      cost.add(objType.action().cost);
      user.removeItems(cost);
    }
  }
}

void Server::handle_CL_TARGET_ENTITY(User &user, Serial serial) {
  if (serial.isInventory() || serial.isGear()) {
    user.setTargetAndAttack(nullptr);
    return;
  }

  user.cancelAction();

  auto target = _entities.find(serial);
  if (!target) {
    sendMessage(user.socket(), WARNING_DOESNT_EXIST);
  }

  else if (target->health() == 0) {
    target = nullptr;
    sendMessage(user.socket(), ERROR_TARGET_DEAD);
  }

  else if (!target->canBeAttackedBy(user)) {
    target = nullptr;
    sendMessage(user.socket(), ERROR_ATTACKED_PEACFUL_PLAYER);
  }

  user.setTargetAndAttack(target);
}

void Server::handle_CL_TARGET_PLAYER(User &user,
                                     const std::string &targetUsername) {
  user.cancelAction();

  auto it = _usersByName.find(targetUsername);
  if (it == _usersByName.end()) RETURN_WITH(ERROR_INVALID_USER)
  User *targetUser = const_cast<User *>(it->second);
  if (targetUser->health() == 0) {
    targetUser = nullptr;
    sendMessage(user.socket(), ERROR_TARGET_DEAD);
  } else if (!_wars.isAtWar(user.name(), targetUsername)) {
    targetUser = nullptr;
    sendMessage(user.socket(), ERROR_ATTACKED_PEACFUL_PLAYER);
  }

  user.setTargetAndAttack(targetUser);
}

void Server::handle_CL_SELECT_ENTITY(User &user, Serial serial) {
  if (serial.isInventory() || serial.isGear()) {
    user.setTargetAndAttack(nullptr);
    return;
  }

  auto target = _entities.find(serial);
  if (target == nullptr) {
    user.setTargetAndAttack(nullptr);
    sendMessage(user.socket(), WARNING_DOESNT_EXIST);
    return;
  }

  user.target(target);
  if (user.action() == User::ATTACK) user.action(User::NO_ACTION);
}

void Server::handle_CL_SELECT_PLAYER(User &user,
                                     const std::string &targetUsername) {
  user.cancelAction();

  auto it = _usersByName.find(targetUsername);
  if (it == _usersByName.end()) RETURN_WITH(ERROR_INVALID_USER)
  User *targetUser = const_cast<User *>(it->second);
  user.target(targetUser);
  if (user.action() == User::ATTACK) user.action(User::NO_ACTION);
}

void Server::handle_CL_RECRUIT(User &user, std::string username) {
  const auto &cityName = _cities.getPlayerCity(user.name());
  if (cityName.empty()) RETURN_WITH(ERROR_NOT_IN_CITY)
  username = toPascal(username);
  if (!_cities.getPlayerCity(username).empty())
    RETURN_WITH(ERROR_ALREADY_IN_CITY)
  const auto *pTargetUser = getUserByName(username);
  if (pTargetUser == nullptr) {
    sendMessage(user.socket(), ERROR_INVALID_USER);
    return;
  }

  _cities.addPlayerToCity(*pTargetUser, cityName);
}

void Server::handle_CL_SUE_FOR_PEACE(User &user, MessageCode code,
                                     const std::string &name) {
  Belligerent proposer, enemy;
  MessageCode codeForProposer, codeForEnemy;
  switch (code) {
    case CL_SUE_FOR_PEACE_WITH_PLAYER:
      proposer = {user.name(), Belligerent::PLAYER};
      enemy = {name, Belligerent::PLAYER};
      codeForProposer = SV_YOU_PROPOSED_PEACE_TO_PLAYER;
      codeForEnemy = SV_PEACE_WAS_PROPOSED_TO_YOU_BY_PLAYER;
      break;
    case CL_SUE_FOR_PEACE_WITH_CITY:
      proposer = {user.name(), Belligerent::PLAYER};
      enemy = {name, Belligerent::CITY};
      codeForProposer = SV_YOU_PROPOSED_PEACE_TO_CITY;
      codeForEnemy = SV_PEACE_WAS_PROPOSED_TO_YOUR_CITY_BY_PLAYER;
      break;
    case CL_SUE_FOR_PEACE_WITH_PLAYER_AS_CITY:
      proposer = {_cities.getPlayerCity(user.name()), Belligerent::CITY};
      enemy = {name, Belligerent::PLAYER};
      codeForProposer = SV_YOUR_CITY_PROPOSED_PEACE_TO_PLAYER;
      codeForEnemy = SV_PEACE_WAS_PROPOSED_TO_YOU_BY_CITY;
      break;
    case CL_SUE_FOR_PEACE_WITH_CITY_AS_CITY:
      proposer = {_cities.getPlayerCity(user.name()), Belligerent::CITY};
      enemy = {name, Belligerent::CITY};
      codeForProposer = SV_YOUR_CITY_PROPOSED_PEACE_TO_CITY;
      codeForEnemy = SV_PEACE_WAS_PROPOSED_TO_YOUR_CITY_BY_CITY;
      break;
  }

  // If a city is proposing, make sure the player is a king
  if (proposer.type == Belligerent::CITY) {
    if (!_cities.isPlayerInACity(user.name())) RETURN_WITH(ERROR_NOT_IN_CITY)
    if (!_kings.isPlayerAKing(user.name())) RETURN_WITH(ERROR_NOT_A_KING)
  }

  _wars.sueForPeace(proposer, enemy);

  // Alert the proposer
  if (proposer.type == Belligerent::PLAYER)
    sendMessage(user.socket(), {codeForProposer, name});
  else
    broadcastToCity(proposer.name, {codeForProposer, name});

  // Alert the enemy
  if (enemy.type == Belligerent::PLAYER)
    sendMessageIfOnline(enemy.name, {codeForEnemy, proposer.name});
  else
    broadcastToCity(enemy.name, {codeForEnemy, proposer.name});
}

void Server::handle_CL_CANCEL_PEACE_OFFER(User &user, MessageCode code,
                                          const std::string &name) {
  Belligerent proposer, enemy;
  MessageCode codeForProposer, codeForEnemy;
  switch (code) {
    case CL_CANCEL_PEACE_OFFER_TO_PLAYER:
      proposer = {user.name(), Belligerent::PLAYER};
      enemy = {name, Belligerent::PLAYER};
      codeForProposer = SV_YOU_CANCELED_PEACE_OFFER_TO_PLAYER;
      codeForEnemy = SV_PEACE_OFFER_TO_YOU_FROM_PLAYER_WAS_CANCELED;
      break;
    case CL_CANCEL_PEACE_OFFER_TO_CITY:
      proposer = {user.name(), Belligerent::PLAYER};
      enemy = {name, Belligerent::CITY};
      codeForProposer = SV_YOU_CANCELED_PEACE_OFFER_TO_CITY;
      codeForEnemy = SV_PEACE_OFFER_TO_YOUR_CITY_FROM_PLAYER_WAS_CANCELED;
      break;
    case CL_CANCEL_PEACE_OFFER_TO_PLAYER_AS_CITY:
      proposer = {_cities.getPlayerCity(user.name()), Belligerent::CITY};
      enemy = {name, Belligerent::PLAYER};
      codeForProposer = SV_YOUR_CITY_CANCELED_PEACE_OFFER_TO_PLAYER;
      codeForEnemy = SV_PEACE_OFFER_TO_YOU_FROM_CITY_WAS_CANCELED;
      break;
    case CL_CANCEL_PEACE_OFFER_TO_CITY_AS_CITY:
      proposer = {_cities.getPlayerCity(user.name()), Belligerent::CITY};
      enemy = {name, Belligerent::CITY};
      codeForProposer = SV_YOUR_CITY_CANCELED_PEACE_OFFER_TO_CITY;
      codeForEnemy = SV_PEACE_OFFER_TO_YOUR_CITY_FROM_CITY_WAS_CANCELED;
      break;
  }

  // If a city is proposing, make sure the player is a king
  if (proposer.type == Belligerent::CITY) {
    if (!_cities.isPlayerInACity(user.name())) RETURN_WITH(ERROR_NOT_IN_CITY)
    if (!_kings.isPlayerAKing(user.name())) RETURN_WITH(ERROR_NOT_A_KING)
  }

  auto canceled = _wars.cancelPeaceOffer(proposer, enemy);
  if (!canceled) return;

  // Alert the proposer
  if (proposer.type == Belligerent::PLAYER)
    sendMessage(user.socket(), {codeForProposer, name});
  else
    broadcastToCity(proposer.name, {codeForProposer, name});

  // Alert the enemy
  if (enemy.type == Belligerent::PLAYER)
    sendMessageIfOnline(enemy.name, {codeForEnemy, proposer.name});
  else
    broadcastToCity(enemy.name, {codeForEnemy, proposer.name});
}

void Server::handle_CL_ACCEPT_PEACE_OFFER(User &user, MessageCode code,
                                          const std::string &name) {
  Belligerent proposer, enemy;
  MessageCode codeForProposer, codeForEnemy;
  switch (code) {
    case CL_ACCEPT_PEACE_OFFER_WITH_PLAYER:
      proposer = {user.name(), Belligerent::PLAYER};
      enemy = {name, Belligerent::PLAYER};
      codeForProposer = SV_AT_PEACE_WITH_PLAYER;
      codeForEnemy = SV_AT_PEACE_WITH_PLAYER;
      break;
    case CL_ACCEPT_PEACE_OFFER_WITH_CITY:
      proposer = {user.name(), Belligerent::PLAYER};
      enemy = {name, Belligerent::CITY};
      codeForProposer = SV_AT_PEACE_WITH_CITY;
      codeForEnemy = SV_YOUR_CITY_IS_AT_PEACE_WITH_PLAYER;
      break;
    case CL_ACCEPT_PEACE_OFFER_WITH_PLAYER_AS_CITY:
      proposer = {_cities.getPlayerCity(user.name()), Belligerent::CITY};
      enemy = {name, Belligerent::PLAYER};
      codeForProposer = SV_YOUR_CITY_IS_AT_PEACE_WITH_PLAYER;
      codeForEnemy = SV_AT_PEACE_WITH_CITY;
      break;
    case CL_ACCEPT_PEACE_OFFER_WITH_CITY_AS_CITY:
      proposer = {_cities.getPlayerCity(user.name()), Belligerent::CITY};
      enemy = {name, Belligerent::CITY};
      codeForProposer = SV_YOUR_CITY_IS_AT_PEACE_WITH_CITY;
      codeForEnemy = SV_YOUR_CITY_IS_AT_PEACE_WITH_CITY;
      break;
  }

  auto accpeted = _wars.acceptPeaceOffer(proposer, enemy);
  if (!accpeted) return;

  // Alert the proposer
  if (proposer.type == Belligerent::PLAYER)
    sendMessage(user.socket(), {codeForProposer, name});
  else
    broadcastToCity(proposer.name, {codeForProposer, name});

  // Alert the enemy
  if (enemy.type == Belligerent::PLAYER)
    sendMessageIfOnline(enemy.name, {codeForEnemy, proposer.name});
  else
    broadcastToCity(enemy.name, {codeForEnemy, proposer.name});
}

void Server::handle_CL_TAKE_TALENT(User &user, const Talent::Name &talentName) {
  auto &userClass = user.getClass();
  const auto &classType = userClass.type();
  auto talent = classType.findTalent(talentName);
  if (talent == nullptr) RETURN_WITH(ERROR_INVALID_TALENT)
  if (!user.getClass().canTakeATalent()) RETURN_WITH(WARNING_NO_TALENT_POINTS)
  if (talent->type() == Talent::SPELL && userClass.hasTalent(talent))
    RETURN_WITH(ERROR_ALREADY_KNOW_SPELL)

  auto &tier = talent->tier();

  if (tier.reqPointsInTree > 0 &&
      user.getClass().pointsInTree(talent->tree()) < tier.reqPointsInTree)
    RETURN_WITH(WARNING_MISSING_REQ_FOR_TALENT)
  if (tier.hasItemCost() && !user.hasItems(tier.costTag, tier.costQuantity))
    RETURN_WITH(WARNING_MISSING_ITEMS_FOR_TALENT)

  // Tool check must be the last check, as it damages the tools.
  const auto &requiredTool = talent->tier().requiredTool;
  auto requiresTool = !requiredTool.empty();
  if (requiresTool && !user.checkAndDamageToolAndGetSpeed(requiredTool)) {
    sendMessage(user.socket(), {WARNING_ITEM_TAG_NEEDED, requiredTool});
    return;
  }

  // All checks must be done by this point.

  if (tier.hasItemCost()) user.removeItems(tier.costTag, tier.costQuantity);

  userClass.takeTalent(talent);

  switch (talent->type()) {
    case Talent::SPELL:
      sendMessage(user.socket(), {SV_LEARNED_SPELL, talent->spellID()});
      break;

    case Talent::STATS:
      user.updateStats();
      break;
  }
}

void Server::handle_CL_UNLEARN_TALENTS(User &user) {
  auto &userClass = user.getClass();
  userClass.unlearnAll();

  auto knownSpellsString = userClass.generateKnownSpellsString();
  user.sendMessage({SV_KNOWN_SPELLS, knownSpellsString});
}

void Server::handle_CL_ACCEPT_QUEST(User &user, const Quest::ID &questID,
                                    Serial startSerial) {
  const auto entity = _entities.find(startSerial);
  if (!entity) return;
  auto node = dynamic_cast<const QuestNode *>(entity);
  if (!node) return;

  if (!isEntityInRange(user.socket(), user, entity)) return;

  if (!node->startsQuest(questID)) return;

  auto quest = findQuest(questID);
  if (!quest) return;
  for (auto prereq : quest->prerequisiteQuests)
    if (!user.hasCompletedQuest(prereq)) return;

  // Enforce class exclusivity
  auto hasExclusiveClass = !quest->exclusiveToClass.empty();
  if (hasExclusiveClass &&
      user.getClass().type().id() != quest->exclusiveToClass)
    return;

  if (!user.hasRoomFor(quest->startsWithItems))
    RETURN_WITH(WARNING_INVENTORY_FULL)

  user.startQuest(*quest);
}

void Server::handle_CL_COMPLETE_QUEST(User &user, const Quest::ID &quest,
                                      Serial endSerial) {
  if (!user.isOnQuest(quest)) return;

  const auto entity = _entities.find(endSerial);
  if (!entity) return;
  auto node = dynamic_cast<const QuestNode *>(entity);
  if (!node) return;

  if (!isEntityInRange(user.socket(), user, entity)) return;

  if (!node->endsQuest(quest)) return;

  const auto it = _quests.find(quest);
  if (it == _quests.end()) return;
  const auto &q = it->second;

  if (!q.canBeCompletedByUser(user)) return;

  user.completeQuest(quest);
}

void Server::handle_CL_ABANDON_QUEST(User &user, const Quest::ID &quest) {
  user.abandonQuest(quest);
}

void Server::handle_CL_AUTO_CONSTRUCT(User &user, Serial serial) {
  auto *obj = _entities.find<Object>(serial);
  if (!obj) return;

  auto materialsToLookFor = ItemSet{};
  for (const auto &pair : obj->remainingMaterials()) {
    const auto *material = dynamic_cast<const ServerItem *>(pair.first);
    auto userWillHaveSpace =
        !material->returnsOnConstruction() || material->stackSize() == 1;

    if (userWillHaveSpace) {
      materialsToLookFor.add(pair.first, pair.second);
      continue;
    }

    // Take a closer look.  Will the user have space for the returned item?
    auto materialToBeRemoved = ItemSet{};
    materialToBeRemoved.add(pair.first, pair.second);
    auto qtyToBeReturned = min<int>(
        pair.second, user.countItems(material->returnsOnConstruction()));
    auto toBeReturned = ItemSet{};
    toBeReturned.add(material->returnsOnConstruction(), qtyToBeReturned);
    if (user.hasRoomToRemoveThenAdd(materialToBeRemoved, toBeReturned))
      materialsToLookFor.add(pair.first, pair.second);
  }

  auto remainder = user.removeItems(materialsToLookFor);
  auto materialsAdded = materialsToLookFor - remainder;
  obj->remainingMaterials().remove(materialsAdded);

  // Return items to user
  for (auto &pair : materialsAdded) {
    const auto *material = dynamic_cast<const ServerItem *>(pair.first);
    auto itemToReturn = material->returnsOnConstruction();
    if (!itemToReturn) continue;
    user.giveItem(itemToReturn, pair.second);
  }

  for (const User *nearbyUser : findUsersInArea(obj->location()))
    sendConstructionMaterialsMessage(*nearbyUser, *obj);

  if (!obj->isBeingBuilt()) {
    // Trigger completing user's unlocks
    if (user.knowsConstruction(obj->type()->id()))
      ProgressLock::triggerUnlocks(user, ProgressLock::CONSTRUCTION,
                                   obj->type());

    // Update quest progress for completing user
    user.addQuestProgress(Quest::Objective::CONSTRUCT, obj->type()->id());
  }
}

void Server::broadcast(const Message &msg) {
  for (const User &user : _users) sendMessage(user.socket(), msg);
}

void Server::broadcastToArea(const MapPoint &location,
                             const Message &msg) const {
  for (const User *user : this->findUsersInArea(location))
    user->sendMessage(msg);
}

void Server::broadcastToCity(const std::string &cityName,
                             const Message &msg) const {
  if (!_cities.doesCityExist(cityName)) {
    SERVER_ERROR("City "s + cityName + " does not exist.");
    return;
  }

  for (const auto &citizen : _cities.membersOf(cityName))
    sendMessageIfOnline(citizen, msg);
}

void Server::broadcastToGroup(Username aMember, const Message &msg) {
  auto group = groups->getUsersGroup(aMember);
  for (auto memberName : group) {
    auto *asUser = getUserByName(memberName);
    asUser->sendMessage(msg);
  }
}

void Server::sendMessage(const Socket &dstSocket, const Message &msg) const {
  _socket.sendMessage(msg, dstSocket);
}

void Server::sendMessageIfOnline(const std::string username,
                                 const Message &msg) const {
  auto it = _usersByName.find(username);
  if (it == _usersByName.end()) return;
  sendMessage(it->second->socket(), msg);
}

void Server::sendInventoryMessageInner(
    const User &user, Serial serial, size_t slot,
    const ServerItem::vect_t &itemVect) const {
  if (slot >= itemVect.size()) RETURN_WITH(ERROR_INVALID_SLOT)

  const auto &containerSlot = itemVect[slot];
  std::string itemID =
      containerSlot.first.hasItem() ? containerSlot.first.type()->id() : "none";
  auto msg = Message{SV_INVENTORY,
                     makeArgs(serial, slot, itemID, containerSlot.second,
                              containerSlot.first.health(),
                              containerSlot.first.isSoulbound() ? 1 : 0)};
  sendMessage(user.socket(), msg);
}

void Server::sendInventoryMessage(const User &user, size_t slot,
                                  const Object &obj) const {
  if (!obj.hasContainer()) {
    SERVER_ERROR("Can't send inventory message for containerless object");
    return;
  }
  sendInventoryMessageInner(user, obj.serial(), slot, obj.container().raw());
}

// Special serials only
void Server::sendInventoryMessage(const User &user, size_t slot,
                                  Serial serial) const {
  const ServerItem::vect_t *container = nullptr;
  if (serial.isInventory())
    container = &user.inventory();
  else if (serial.isGear())
    container = &user.gear();
  else {
    SERVER_ERROR(
        "Trying to send inventory message with bad serial.  Using "
        "inventory.");
    container = &user.inventory();
  }
  sendInventoryMessageInner(user, serial, slot, *container);
}

void Server::sendMerchantSlotMessage(const User &user, const Object &obj,
                                     size_t slot) const {
  if (slot >= obj.merchantSlots().size()) {
    SERVER_ERROR("Can't send merchant-slot message: slot index is too high");
    return;
  }
  const MerchantSlot &mSlot = obj.merchantSlot(slot);
  if (mSlot)
    sendMessage(
        user.socket(),
        {SV_MERCHANT_SLOT,
         makeArgs(obj.serial(), slot, mSlot.wareItem->id(), mSlot.wareQty,
                  mSlot.priceItem->id(), mSlot.priceQty)});
  else
    sendMessage(user.socket(),
                {SV_MERCHANT_SLOT, makeArgs(obj.serial(), slot, "", 0, "", 0)});
}

void Server::sendConstructionMaterialsMessage(const User &user,
                                              const Object &obj) const {
  size_t n = obj.remainingMaterials().numTypes();
  std::string args = makeArgs(obj.serial(), n);
  for (const auto &pair : obj.remainingMaterials()) {
    args = makeArgs(args, pair.first->id(), pair.second);
  }
  sendMessage(user.socket(), {SV_CONSTRUCTION_MATERIALS, args});
}

void Server::sendNewBuildsMessage(const User &user,
                                  const std::set<std::string> &ids) const {
  if (!ids.empty()) {  // New constructions unlocked!
    std::string args = makeArgs(ids.size());
    for (const std::string &id : ids) args = makeArgs(args, id);
    sendMessage(user.socket(), {SV_NEW_CONSTRUCTIONS, args});
  }
}

void Server::sendNewRecipesMessage(const User &user,
                                   const std::set<std::string> &ids) const {
  if (!ids.empty()) {  // New recipes unlocked!
    std::string args = makeArgs(ids.size());
    for (const std::string &id : ids) args = makeArgs(args, id);
    sendMessage(user.socket(), {SV_NEW_RECIPES, args});
  }
}

void Server::alertUserToWar(const std::string &username,
                            const Belligerent &otherBelligerent,
                            bool isUserCityTheBelligerent) const {
  auto it = _usersByName.find(username);
  if (it == _usersByName.end())  // user is offline
    return;

  MessageCode code;
  if (isUserCityTheBelligerent)
    code = otherBelligerent.type == Belligerent::CITY
               ? SV_YOUR_CITY_AT_WAR_WITH_CITY
               : SV_YOUR_CITY_AT_WAR_WITH_PLAYER;
  else
    code = otherBelligerent.type == Belligerent::CITY ? SV_AT_WAR_WITH_CITY
                                                      : SV_AT_WAR_WITH_PLAYER;
  sendMessage(it->second->socket(), {code, otherBelligerent.name});
}

void Server::sendRelevantEntitiesToUser(const User &user) {
  std::set<const Entity *>
      entitiesToDescribe;  // Multiple sources; a set ensures no duplicates.

  // (Nearby)
  const MapPoint &loc = user.location();
  auto loX =
      _entitiesByX.lower_bound(&Dummy::Location(loc.x - CULL_DISTANCE, 0));
  auto hiX =
      _entitiesByX.upper_bound(&Dummy::Location(loc.x + CULL_DISTANCE, 0));
  for (auto it = loX; it != hiX; ++it) {
    const Entity *entity = *it;
    if (abs(entity->location().y - loc.y) > CULL_DISTANCE)  // Cull y
      continue;
    entitiesToDescribe.insert(entity);
  }

  // (Owned objects)
  for (auto pEntity : _entities) {
    if (pEntity->spawner()) continue;  // Optimisation and assumption

    auto *pObject = dynamic_cast<const Object *>(pEntity);
    if (!pObject) continue;

    // Player owns directly
    auto userOwnsThisObject = _objectsByOwner.isObjectOwnedBy(
        pEntity->serial(), {Permissions::Owner::PLAYER, user.name()});
    if (userOwnsThisObject) {
      entitiesToDescribe.insert(pEntity);

      // Object-specific stuff
      auto *pObject = dynamic_cast<const Object *>(pEntity);
      if (!pObject && !pEntity->isDead())
        user.registerObjectIfPlayerUnique(pObject->objType());

      continue;
    }

    // City owns
    if (!_cities.isPlayerInACity(user.name())) continue;
    auto cityOwnsThisObject = _objectsByOwner.isObjectOwnedBy(
        pEntity->serial(),
        {Permissions::Owner::CITY, _cities.getPlayerCity(user.name())});
    if (cityOwnsThisObject) entitiesToDescribe.insert(pEntity);
  }

  // Send
  for (const Entity *entity : entitiesToDescribe) {
    if (!entity->type()) {
      _debug("Null-type object skipped", Color::CHAT_ERROR);
      continue;
    }
    if (entity->shouldBePropagatedToClients()) entity->sendInfoToClient(user);
  }
}

void Server::sendOnlineUsersTo(const User &recipient) const {
  auto numNames = 0;
  auto args = ""s;
  for (auto &user : _users) {
    if (user.name() == recipient.name()) continue;

    args = makeArgs(args, user.name());
    ++numNames;
  }

  if (numNames == 0) return;

  args = makeArgs(numNames, args);
  recipient.sendMessage({SV_USERS_ALREADY_ONLINE, args});
}
