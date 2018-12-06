#include <algorithm>
#include <cassert>
#include <set>

#include "../messageCodes.h"
#include "../versionUtil.h"
#include "ProgressLock.h"
#include "Server.h"
#include "Vehicle.h"
#include "objects/Deconstruction.h"

void Server::handleMessage(const Socket &client, const std::string &msg) {
  _debug(msg);
  int msgCode;
  char del;
  static char buffer[BUFFER_SIZE + 1];
  std::istringstream iss(msg);
  User *user = nullptr;
  while (iss.peek() == MSG_START) {
    iss >> del >> msgCode >> del;

    // Discard message if this client has not yet logged in
    const auto messagesAllowedBeforeLogin =
        std::set<int>{CL_PING, CL_LOGIN_EXISTING, CL_LOGIN_NEW};
    auto thisMessageIsAllowedBeforeLogin =
        messagesAllowedBeforeLogin.find(msgCode) !=
        messagesAllowedBeforeLogin.end();
    auto it = _users.find(client);
    auto userHasLoggedIn = it != _users.end();
    if (!userHasLoggedIn && !thisMessageIsAllowedBeforeLogin) {
      continue;
    }

    if (userHasLoggedIn) {
      User &userRef = const_cast<User &>(*it);
      user = &userRef;
      user->contact();
    }

    switch (msgCode) {
      case CL_PING: {
        ms_t timeSent;
        iss >> timeSent >> del;
        if (del != MSG_END) return;
        sendMessage(client, SV_PING_REPLY, makeArgs(timeSent));
        break;
      }

      case CL_LOGIN_EXISTING: {
        iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
        auto name = std::string(buffer);
        iss >> del;

        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto clientVersion = std::string(buffer);
        iss >> del;
        if (del != MSG_END) return;

        handle_CL_LOGIN_EXISTING(client, name, clientVersion);
        break;
      }

      case CL_LOGIN_NEW: {
        iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
        auto name = std::string(buffer);
        iss >> del;

        iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
        auto classID = std::string(buffer);
        iss >> del;

        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto clientVersion = std::string(buffer);
        iss >> del;
        if (del != MSG_END) return;

        handle_CL_LOGIN_NEW(client, name, classID, clientVersion);
        break;
      }

      case CL_LOCATION: {
        double x, y;
        iss >> x >> del >> y >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(
              client, SV_LOCATION,
              makeArgs(user->name(), user->location().x, user->location().y));
          break;
        }

        if (user->action() != User::ATTACK) user->cancelAction();
        if (user->isDriving()) {
          // Move vehicle and user together
          size_t vehicleSerial = user->driving();
          Vehicle &vehicle = *_entities.find<Vehicle>(vehicleSerial);
          vehicle.updateLocation({x, y});
          user->updateLocation(vehicle.location());
          break;
        }
        user->updateLocation({x, y});
        break;
      }

      case CL_CRAFT: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        std::string id(buffer);
        iss >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        user->cancelAction();
        const std::set<Recipe>::const_iterator it = _recipes.find(id);
        if (!user->knowsRecipe(id)) {
          sendMessage(client, ERROR_UNKNOWN_RECIPE);
          break;
        }
        ItemSet remaining;
        if (!user->hasItems(it->materials())) {
          sendMessage(client, WARNING_NEED_MATERIALS);
          break;
        }
        if (!user->hasTools(it->tools())) {
          sendMessage(client, WARNING_NEED_TOOLS);
          break;
        }
        user->beginCrafting(*it);
        sendMessage(client, SV_ACTION_STARTED, makeArgs(it->time()));
        break;
      }

      case CL_CONSTRUCT: {
        double x, y;
        iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
        std::string id(buffer);
        iss >> del >> x >> del >> y >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        user->cancelAction();
        const ObjectType *objType = findObjectTypeByName(id);
        if (objType == nullptr) {
          sendMessage(client, ERROR_INVALID_OBJECT);
          break;
        }
        if (!user->knowsConstruction(id)) {
          sendMessage(client, ERROR_UNKNOWN_CONSTRUCTION);
          break;
        }
        if (objType->isUnique() && objType->numInWorld() == 1) {
          sendMessage(client, WARNING_UNIQUE_OBJECT);
          break;
        }
        if (objType->isPlayerUnique() &&
            user->hasPlayerUnique(objType->playerUniqueCategory())) {
          sendMessage(client, WARNING_PLAYER_UNIQUE_OBJECT,
                      objType->playerUniqueCategory());
          break;
        }
        if (objType->isUnbuildable()) {
          sendMessage(client, ERROR_UNBUILDABLE);
          break;
        }
        bool requiresTool = !objType->constructionReq().empty();
        if (requiresTool && !user->hasTool(objType->constructionReq())) {
          sendMessage(client, WARNING_NEED_TOOLS);
          break;
        }
        const MapPoint location(x, y);
        if (distance(user->collisionRect(),
                     objType->collisionRect() + location) > ACTION_DISTANCE) {
          sendMessage(client, WARNING_TOO_FAR);
          break;
        }
        if (!isLocationValid(location, *objType)) {
          sendMessage(client, WARNING_BLOCKED);
          break;
        }
        user->beginConstructing(*objType, location);
        sendMessage(client, SV_ACTION_STARTED,
                    makeArgs(objType->constructionTime()));
        break;
      }

      case CL_CONSTRUCT_ITEM: {
        size_t slot;
        double x, y;
        iss >> slot >> del >> x >> del >> y >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        user->cancelAction();
        if (slot >= User::INVENTORY_SIZE) {
          sendMessage(client, ERROR_INVALID_SLOT);
          break;
        }
        const std::pair<const ServerItem *, size_t> &invSlot =
            user->inventory(slot);
        if (invSlot.first == nullptr) {
          sendMessage(client, ERROR_EMPTY_SLOT);
          break;
        }
        const ServerItem &item = *invSlot.first;
        if (item.constructsObject() == nullptr) {
          sendMessage(client, ERROR_CANNOT_CONSTRUCT);
          break;
        }
        const MapPoint location(x, y);
        const ObjectType &objType = *item.constructsObject();
        if (distance(user->collisionRect(),
                     objType.collisionRect() + location) > ACTION_DISTANCE) {
          sendMessage(client, WARNING_TOO_FAR);
          break;
        }
        if (!isLocationValid(location, objType)) {
          sendMessage(client, WARNING_BLOCKED);
          break;
        }
        const std::string constructionReq = objType.constructionReq();
        if (!(constructionReq.empty() || user->hasTool(constructionReq))) {
          sendMessage(client, WARNING_ITEM_TAG_NEEDED, constructionReq);
          break;
        }
        user->beginConstructing(objType, location, slot);
        sendMessage(client, SV_ACTION_STARTED,
                    makeArgs(objType.constructionTime()));
        break;
      }

      case CL_CAST_ITEM: {
        size_t slot;
        iss >> slot >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        user->cancelAction();
        if (slot >= User::INVENTORY_SIZE) {
          sendMessage(client, ERROR_INVALID_SLOT);
          break;
        }
        std::pair<const ServerItem *, size_t> &invSlot = user->inventory(slot);
        if (invSlot.first == nullptr) {
          sendMessage(client, ERROR_EMPTY_SLOT);
          break;
        }
        const ServerItem &item = *invSlot.first;
        if (!item.castsSpellOnUse()) {
          sendMessage(client, ERROR_CANNOT_CAST_ITEM);
          break;
        }

        auto spellID = item.spellToCastOnUse();
        auto result =
            handle_CL_CAST(*user, spellID, /* casting from item*/ true);

        if (result == FAIL) break;

        --invSlot.second;
        if (invSlot.second == 0) invSlot.first = nullptr;
        ;
        sendInventoryMessage(*user, slot, INVENTORY);

        break;
      }

      case CL_CANCEL_ACTION: {
        iss >> del;
        if (del != MSG_END) return;
        user->cancelAction();
        break;
      }

      case CL_GATHER: {
        int serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        user->cancelAction();
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) {
          // isEntityInRange() sends error messages if applicable.
          break;
        }
        assert(obj->type() != nullptr);
        // Check that the user meets the requirements
        if (obj->isBeingBuilt()) {
          sendMessage(client, ERROR_UNDER_CONSTRUCTION);
          break;
        }
        if (!obj->permissions().doesUserHaveAccess(user->name())) {
          sendMessage(client, WARNING_NO_PERMISSION);
          break;
        }
        const std::string &gatherReq = obj->objType().gatherReq();
        if (gatherReq != "none" && !user->hasTool(gatherReq)) {
          sendMessage(client, WARNING_ITEM_TAG_NEEDED, gatherReq);
          break;
        }
        // Check that it has an inventory
        if (obj->hasContainer() && !obj->container().isEmpty()) {
          sendMessage(client, WARNING_NOT_EMPTY);
          break;
        }
        user->beginGathering(obj);
        sendMessage(client, SV_ACTION_STARTED,
                    makeArgs(obj->objType().gatherTime()));
        break;
      }

      case CL_DECONSTRUCT: {
        int serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        user->cancelAction();
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) {
          sendMessage(client, WARNING_TOO_FAR);
          break;
        }
        if (obj->isBeingBuilt()) {
          sendMessage(client, ERROR_UNDER_CONSTRUCTION);
          break;
        }
        assert(obj->type());
        if (!obj->permissions().doesUserHaveAccess(user->name())) {
          sendMessage(client, WARNING_NO_PERMISSION);
          break;
        }
        // Check that the object can be deconstructed
        if (!obj->hasDeconstruction()) {
          sendMessage(client, ERROR_CANNOT_DECONSTRUCT);
          break;
        }
        if (obj->health() < obj->stats().maxHealth) {
          sendMessage(client, ERROR_DAMAGED_OBJECT);
          break;
        }
        if (!obj->isAbleToDeconstruct(*user)) {
          break;
        }
        // Check that it isn't an occupied vehicle
        if (obj->classTag() == 'v' &&
            !dynamic_cast<const Vehicle *>(obj)->driver().empty()) {
          sendMessage(client, WARNING_VEHICLE_OCCUPIED);
          break;
        }

        user->beginDeconstructing(*obj);
        sendMessage(client, SV_ACTION_STARTED,
                    makeArgs(obj->deconstruction().timeToDeconstruct()));
        break;
      }

      case CL_DEMOLISH: {
        int serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        user->cancelAction();
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) {
          sendMessage(client, WARNING_TOO_FAR);
          break;
        }
        assert(obj->type());
        if (!obj->permissions().isOwnedByPlayer(user->name())) {
          sendMessage(client, WARNING_NO_PERMISSION);
          break;
        }
        // Check that it isn't an occupied vehicle
        if (obj->classTag() == 'v' &&
            !dynamic_cast<const Vehicle *>(obj)->driver().empty()) {
          sendMessage(client, WARNING_VEHICLE_OCCUPIED);
          break;
        }

        obj->kill();
        break;
      }

      case CL_DROP: {
        size_t serial, slot;
        iss >> serial >> del >> slot >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        ServerItem::vect_t *container;
        Object *pObj = nullptr;
        bool breakMsg = false;
        switch (serial) {
          case INVENTORY:
            container = &user->inventory();
            break;
          case GEAR:
            container = &user->gear();
            break;
          default:
            pObj = _entities.find<Object>(serial);
            if (!pObj->hasContainer()) {
              sendMessage(client, ERROR_NO_INVENTORY);
              breakMsg = true;
              break;
            }
            if (!isEntityInRange(client, *user, pObj)) {
              breakMsg = true;
              break;
            }
            container = &pObj->container().raw();
        }
        if (breakMsg) break;

        if (slot >= container->size()) {
          sendMessage(client, ERROR_INVALID_SLOT);
          break;
        }
        auto &containerSlot = (*container)[slot];
        if (containerSlot.second != 0) {
          containerSlot.first = nullptr;
          containerSlot.second = 0;

          // Alert relevant users
          if (serial == INVENTORY || serial == GEAR)
            sendInventoryMessage(*user, slot, serial);
          else
            for (auto username : pObj->watchers())
              if (pObj->permissions().doesUserHaveAccess(username))
                sendInventoryMessage(*_usersByName[username], slot, *pObj);
        }
        break;
      }

      case CL_SWAP_ITEMS: {
        size_t obj1, slot1, obj2, slot2;
        iss >> obj1 >> del >> slot1 >> del >> obj2 >> del >> slot2 >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        ServerItem::vect_t *containerFrom = nullptr, *containerTo = nullptr;
        Object *pObj1 = nullptr, *pObj2 = nullptr;
        bool breakMsg = false;
        switch (obj1) {
          case INVENTORY:
            containerFrom = &user->inventory();
            break;
          case GEAR:
            containerFrom = &user->gear();
            break;
          default:
            pObj1 = _entities.find<Object>(obj1);
            if (!pObj1->hasContainer()) {
              sendMessage(client, ERROR_NO_INVENTORY);
              breakMsg = true;
              break;
            }
            if (!isEntityInRange(client, *user, pObj1)) {
              sendMessage(client, WARNING_TOO_FAR);
              breakMsg = true;
            }
            if (!pObj1->permissions().doesUserHaveAccess(user->name())) {
              sendMessage(client, WARNING_NO_PERMISSION);
              breakMsg = true;
            }
            containerFrom = &pObj1->container().raw();
        }
        if (breakMsg) break;
        bool isConstructionMaterial = false;
        switch (obj2) {
          case INVENTORY:
            containerTo = &user->inventory();
            break;
          case GEAR:
            containerTo = &user->gear();
            break;
          default:
            pObj2 = _entities.find<Object>(obj2);
            if (pObj2 != nullptr && pObj2->isBeingBuilt() && slot2 == 0)
              isConstructionMaterial = true;
            if (!isConstructionMaterial && !pObj2->hasContainer()) {
              sendMessage(client, ERROR_NO_INVENTORY);
              breakMsg = true;
              break;
            }
            if (!isEntityInRange(client, *user, pObj2)) {
              sendMessage(client, WARNING_TOO_FAR);
              breakMsg = true;
            }
            if (!pObj2->permissions().doesUserHaveAccess(user->name())) {
              sendMessage(client, WARNING_NO_PERMISSION);
              breakMsg = true;
            }
            containerTo = &pObj2->container().raw();
        }
        if (breakMsg) break;

        if (slot1 >= containerFrom->size() ||
            !isConstructionMaterial && slot2 >= containerTo->size() ||
            isConstructionMaterial && slot2 > 0) {
          sendMessage(client, ERROR_INVALID_SLOT);
          break;
        }

        auto &slotFrom = (*containerFrom)[slot1];
        assert(slotFrom.first != nullptr);

        if (isConstructionMaterial) {
          size_t qtyInSlot = slotFrom.second,
                 qtyNeeded = pObj2->remainingMaterials()[slotFrom.first],
                 qtyToTake = min(qtyInSlot, qtyNeeded);

          if (qtyNeeded == 0) {
            sendMessage(client, WARNING_WRONG_MATERIAL);
            break;
          }

          auto itemToReturn = slotFrom.first->returnsOnConstruction();

          // Remove from object requirements
          pObj2->remainingMaterials().remove(slotFrom.first, qtyToTake);
          for (auto username : pObj2->watchers())
            if (pObj2->permissions().doesUserHaveAccess(username))
              sendConstructionMaterialsMessage(*_usersByName[username], *pObj2);

          // Remove items from user
          slotFrom.second -= qtyToTake;
          if (slotFrom.second == 0) slotFrom.first = nullptr;
          sendInventoryMessage(*user, slot1, obj1);

          // Check if this action completed construction
          if (!pObj2->isBeingBuilt()) {
            // Send to all nearby players, since object appearance will change
            for (const User *otherUser : findUsersInArea(user->location()))
              sendConstructionMaterialsMessage(*otherUser, *pObj2);
            for (const std::string &owner :
                 pObj2->permissions().ownerAsUsernames()) {
              auto pUser = getUserByName(owner);
              if (pUser)
                sendConstructionMaterialsMessage(pUser->socket(), *pObj2);
            }

            // Trigger completing user's unlocks
            if (user->knowsConstruction(pObj2->type()->id()))
              ProgressLock::triggerUnlocks(*user, ProgressLock::CONSTRUCTION,
                                           pObj2->type());

            // Update quest progress for completing user
            user->addQuestProgress(Quest::Objective::CONSTRUCT,
                                   pObj2->type()->id());
          }

          // Return an item to the user, if required.
          if (itemToReturn != nullptr) user->giveItem(itemToReturn);

          break;
        }

        auto &slotTo = (*containerTo)[slot2];

        if (pObj1 != nullptr && pObj1->classTag() == 'n' &&
                slotTo.first != nullptr ||
            pObj2 != nullptr && pObj2->classTag() == 'n' &&
                slotFrom.first != nullptr) {
          sendMessage(client, ERROR_NPC_SWAP);
          break;
        }

        // Check gear-slot compatibility
        if (obj1 == GEAR && slotTo.first != nullptr &&
                slotTo.first->gearSlot() != slot1 ||
            obj2 == GEAR && slotFrom.first != nullptr &&
                slotFrom.first->gearSlot() != slot2) {
          sendMessage(client, ERROR_NOT_GEAR);
          break;
        }

        // Combine stack, if identical types
        auto shouldPerformNormalSwap = true;
        do {
          if (slotFrom.first == nullptr || slotTo.first == nullptr) break;
          auto identicalItems = slotFrom.first == slotTo.first;
          if (!identicalItems) break;
          auto roomInDest = slotTo.first->stackSize() - slotTo.second;
          if (roomInDest == 0) break;

          auto qtyToMove = min(roomInDest, slotFrom.second);
          slotFrom.second -= qtyToMove;
          slotTo.second += qtyToMove;
          if (slotFrom.second == 0) slotFrom.first = nullptr;
          shouldPerformNormalSwap = false;

        } while (false);

        if (shouldPerformNormalSwap) {
          // Perform the swap
          auto temp = slotTo;
          slotTo = slotFrom;
          slotFrom = temp;

          // If gear was changed
          if (obj1 == GEAR || obj2 == GEAR) {
            // Update this player's stats
            user->updateStats();

            // Alert nearby users of the new equipment
            // Assumption: gear can only match a single gear slot.
            std::string gearID = "";
            size_t gearSlot;
            if (obj1 == GEAR) {
              gearSlot = slot1;
              if (slotFrom.first != nullptr) gearID = slotFrom.first->id();
            } else {
              gearSlot = slot2;
              if (slotTo.first != nullptr) gearID = slotTo.first->id();
            }
            for (const User *otherUser : findUsersInArea(user->location())) {
              if (otherUser == user) continue;
              sendMessage(otherUser->socket(), SV_GEAR,
                          makeArgs(user->name(), gearSlot, gearID));
            }
          }
        }

        // Alert relevant users
        if (obj1 == INVENTORY || obj1 == GEAR)
          sendInventoryMessage(*user, slot1, obj1);
        else
          for (auto username : pObj1->watchers())
            if (pObj1->permissions().doesUserHaveAccess(username))
              sendInventoryMessage(*_usersByName[username], slot1, *pObj1);
        if (obj2 == INVENTORY || obj2 == GEAR) {
          sendInventoryMessage(*user, slot2, obj2);
          ProgressLock::triggerUnlocks(*user, ProgressLock::ITEM, slotTo.first);
        } else
          for (auto username : pObj2->watchers())
            if (pObj2->permissions().doesUserHaveAccess(username))
              sendInventoryMessage(*_usersByName[username], slot2, *pObj2);

        break;
      }

      case CL_TAKE_ITEM: {
        size_t serial, slotNum;
        iss >> serial >> del >> slotNum >> del;
        if (del != MSG_END) return;

        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }

        handle_CL_TAKE_ITEM(*user, serial, slotNum);
        break;
      }

      case CL_TRADE: {
        size_t serial, slot;
        iss >> serial >> del >> slot >> del;
        if (del != MSG_END) return;

        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }

        // Check that merchant slot is valid
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) break;
        if (obj->isBeingBuilt()) {
          sendMessage(client, ERROR_UNDER_CONSTRUCTION);
          break;
        }
        size_t slots = obj->objType().merchantSlots();
        if (slots == 0) {
          sendMessage(client, ERROR_NOT_MERCHANT);
          break;
        } else if (slot >= slots) {
          sendMessage(client, ERROR_INVALID_MERCHANT_SLOT);
          break;
        }
        const MerchantSlot &mSlot = obj->merchantSlot(slot);
        if (!mSlot) {
          sendMessage(client, ERROR_INVALID_MERCHANT_SLOT);
          break;
        }

        // Check that user has price
        if (mSlot.price() > user->inventory()) {
          sendMessage(client, WARNING_NO_PRICE);
          break;
        }

        // Check that user has inventory space
        if (!obj->hasContainer() && !obj->objType().bottomlessMerchant()) {
          sendMessage(client, ERROR_NO_INVENTORY);
          break;
        }
        auto wareItem = toServerItem(mSlot.wareItem);
        if (!vectHasSpace(user->inventory(), wareItem, mSlot.wareQty)) {
          sendMessage(client, WARNING_INVENTORY_FULL);
          break;
        }

        bool bottomless = obj->objType().bottomlessMerchant();
        if (!bottomless) {
          // Check that object has items in stock
          if (mSlot.ware() > obj->container().raw()) {
            sendMessage(client, WARNING_NO_WARE);
            break;
          }

          // Check that object has inventory space
          auto priceItem = toServerItem(mSlot.priceItem);
          if (!vectHasSpace(obj->container().raw(), priceItem,
                            mSlot.priceQty)) {
            sendMessage(client, WARNING_MERCHANT_INVENTORY_FULL);
            break;
          }
        }

        // Take price from user
        user->removeItems(mSlot.price());

        if (!bottomless) {
          // Take ware from object
          obj->container().removeItems(mSlot.ware());

          // Give price to object
          obj->container().addItems(toServerItem(mSlot.priceItem),
                                    mSlot.priceQty);
        }

        // Give ware to user
        user->giveItem(wareItem, mSlot.wareQty);

        break;
      }

      case CL_SET_MERCHANT_SLOT: {
        size_t serial, slot, wareQty, priceQty;
        iss >> serial >> del >> slot >> del;
        iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
        std::string ware(buffer);
        iss >> del >> wareQty >> del;
        iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
        std::string price(buffer);
        iss >> del >> priceQty >> del;
        if (del != MSG_END) return;
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) break;
        if (obj->isBeingBuilt()) {
          sendMessage(client, ERROR_UNDER_CONSTRUCTION);
          break;
        }
        if (!obj->permissions().doesUserHaveAccess(user->name())) {
          sendMessage(client, WARNING_NO_PERMISSION);
          break;
        }
        size_t slots = obj->objType().merchantSlots();
        if (slots == 0) {
          sendMessage(client, ERROR_NOT_MERCHANT);
          break;
        }
        if (slot >= slots) {
          sendMessage(client, ERROR_INVALID_MERCHANT_SLOT);
          break;
        }
        auto wareIt = _items.find(ware);
        if (wareIt == _items.end()) {
          sendMessage(client, ERROR_INVALID_ITEM);
          break;
        }
        auto priceIt = _items.find(price);
        if (priceIt == _items.end()) {
          sendMessage(client, ERROR_INVALID_ITEM);
          break;
        }
        MerchantSlot &mSlot = obj->merchantSlot(slot);
        mSlot = MerchantSlot(&*wareIt, wareQty, &*priceIt, priceQty);

        // Alert watchers
        for (auto username : obj->watchers())
          sendMerchantSlotMessage(*_usersByName[username], *obj, slot);
        break;
      }

      case CL_CLEAR_MERCHANT_SLOT: {
        size_t serial, slot;
        iss >> serial >> del >> slot >> del;
        if (del != MSG_END) return;
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) break;
        if (obj->isBeingBuilt()) {
          sendMessage(client, ERROR_UNDER_CONSTRUCTION);
          break;
        }
        if (!obj->permissions().doesUserHaveAccess(user->name())) {
          sendMessage(client, WARNING_NO_PERMISSION);
          break;
        }
        size_t slots = obj->objType().merchantSlots();
        if (slots == 0) {
          sendMessage(client, ERROR_NOT_MERCHANT);
          break;
        }
        if (slot >= slots) {
          sendMessage(client, ERROR_INVALID_MERCHANT_SLOT);
          break;
        }
        obj->merchantSlot(slot) = MerchantSlot();

        // Alert watchers
        for (auto username : obj->watchers())
          sendMerchantSlotMessage(*_usersByName[username], *obj, slot);
        break;
      }

      case CL_MOUNT: {
        size_t serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        Object *obj = _entities.find<Object>(serial);
        if (!isEntityInRange(client, *user, obj)) break;
        if (obj->isBeingBuilt()) {
          sendMessage(client, ERROR_UNDER_CONSTRUCTION);
          break;
        }
        if (!obj->permissions().doesUserHaveAccess(user->name())) {
          sendMessage(client, WARNING_NO_PERMISSION);
          break;
        }
        if (obj->classTag() != 'v') {
          sendMessage(client, ERROR_NOT_VEHICLE);
          break;
        }
        Vehicle *v = dynamic_cast<Vehicle *>(obj);
        if (!v->driver().empty() && v->driver() != user->name()) {
          sendMessage(client, WARNING_VEHICLE_OCCUPIED);
          break;
        }

        v->driver(user->name());
        user->driving(v->serial());
        user->updateLocation(v->location());
        // Alert nearby users (including the new driver)
        for (const User *u : findUsersInArea(user->location()))
          sendMessage(u->socket(), SV_MOUNTED, makeArgs(serial, user->name()));

        break;
      }

      case CL_DISMOUNT: {
        MapPoint target;
        iss >> target.x >> del >> target.y >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        if (!user->isDriving()) {
          sendMessage(client, ERROR_NOT_VEHICLE);
          break;
        }
        auto dstRect = user->type()->collisionRect() + target;
        if (distance(dstRect, user->collisionRect()) > ACTION_DISTANCE) {
          sendMessage(client, WARNING_TOO_FAR);
          break;
        }
        if (!isLocationValid(target, User::OBJECT_TYPE)) {
          sendMessage(client, WARNING_BLOCKED);
          break;
        }
        size_t serial = user->driving();
        Object *obj = _entities.find<Object>(serial);
        Vehicle *v = dynamic_cast<Vehicle *>(obj);

        // Move him before dismounting him, to avoid unnecessary
        // collision/distance checks
        user->updateLocation(target);

        v->driver("");
        user->driving(0);
        for (const User *u : findUsersInArea(user->location()))
          sendMessage(u->socket(), SV_UNMOUNTED,
                      makeArgs(serial, user->name()));

        break;
      }

      case CL_START_WATCHING: {
        size_t serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        handle_CL_START_WATCHING(*user, serial);
        break;
      }

      case CL_STOP_WATCHING: {
        size_t serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        Object *obj = _entities.find<Object>(serial);
        if (obj == nullptr) {
          sendMessage(client, WARNING_DOESNT_EXIST);
          break;
        }

        obj->removeWatcher(user->name());

        break;
      }

      case CL_TARGET_ENTITY: {
        size_t serial;
        iss >> serial >> del;
        if (del != MSG_END) return;

        handle_CL_TARGET_ENTITY(*user, serial);
        break;
      }

      case CL_TARGET_PLAYER: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        std::string targetUsername(buffer);
        iss >> del;
        if (del != MSG_END) return;

        handle_CL_TARGET_PLAYER(*user, targetUsername);
        break;
      }

      case CL_SELECT_ENTITY: {
        size_t serial;
        iss >> serial >> del;
        if (del != MSG_END) return;

        handle_CL_SELECT_ENTITY(*user, serial);
        break;
      }

      case CL_SELECT_PLAYER: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        std::string targetUsername(buffer);
        iss >> del;
        if (del != MSG_END) return;

        handle_CL_SELECT_PLAYER(*user, targetUsername);
        break;
      }

      case CL_DECLARE_WAR_ON_PLAYER:
      case CL_DECLARE_WAR_ON_CITY: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        std::string targetName(buffer);
        iss >> del;
        if (del != MSG_END) return;
        auto thisBelligerent = Belligerent{user->name(), Belligerent::PLAYER};
        Belligerent::Type targetType = msgCode == CL_DECLARE_WAR_ON_PLAYER
                                           ? Belligerent::PLAYER
                                           : Belligerent::CITY;
        auto targetBelligerent = Belligerent{targetName, targetType};

        if (_wars.isAtWarExact(user->name(), targetBelligerent)) {
          sendMessage(client, ERROR_ALREADY_AT_WAR);
          break;
        }

        _wars.declare(thisBelligerent, targetBelligerent);
        break;
      }

      case CL_DECLARE_WAR_ON_PLAYER_AS_CITY:
      case CL_DECLARE_WAR_ON_CITY_AS_CITY: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        std::string targetName(buffer);
        iss >> del;
        if (del != MSG_END) return;

        if (!_cities.isPlayerInACity(user->name())) {
          sendMessage(client, ERROR_NOT_IN_CITY);
          break;
        }
        if (!_kings.isPlayerAKing(user->name())) {
          sendMessage(client, ERROR_NOT_A_KING);
          break;
        }
        auto declarer =
            Belligerent{_cities.getPlayerCity(user->name()), Belligerent::CITY};

        Belligerent::Type targetType =
            msgCode == CL_DECLARE_WAR_ON_PLAYER_AS_CITY ? Belligerent::PLAYER
                                                        : Belligerent::CITY;
        auto targetBelligerent = Belligerent{targetName, targetType};

        if (_wars.isAtWarExact(declarer, targetBelligerent)) {
          sendMessage(client, ERROR_ALREADY_AT_WAR);
          break;
        }

        _wars.declare(declarer, targetBelligerent);
        break;
      }

      case CL_SUE_FOR_PEACE_WITH_PLAYER:
      case CL_SUE_FOR_PEACE_WITH_CITY:
      case CL_SUE_FOR_PEACE_WITH_PLAYER_AS_CITY:
      case CL_SUE_FOR_PEACE_WITH_CITY_AS_CITY: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto name = std::string{buffer};
        iss >> del;
        if (del != MSG_END) return;
        handle_CL_SUE_FOR_PEACE(*user, static_cast<MessageCode>(msgCode), name);
        break;
      }

      case CL_CANCEL_PEACE_OFFER_TO_PLAYER:
      case CL_CANCEL_PEACE_OFFER_TO_CITY:
      case CL_CANCEL_PEACE_OFFER_TO_PLAYER_AS_CITY:
      case CL_CANCEL_PEACE_OFFER_TO_CITY_AS_CITY: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto name = std::string{buffer};
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
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto name = std::string{buffer};
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

      case CL_CEDE: {
        size_t serial;
        iss >> serial >> del;
        if (del != MSG_END) return;
        handle_CL_CEDE(*user, serial);
        break;
      }

      case CL_GRANT: {
        auto serial = size_t{};
        iss >> serial >> del;
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto username = std::string{buffer};
        iss >> del;
        if (del != MSG_END) return;
        handle_CL_GRANT(*user, serial, username);
        break;
      }

      case CL_PERFORM_OBJECT_ACTION: {
        size_t serial;
        iss >> serial >> del;
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto textArg = std::string{buffer};
        iss >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        handle_CL_PERFORM_OBJECT_ACTION(*user, serial, textArg);

        break;
      }

      case CL_RECRUIT: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto username = std::string{buffer};
        iss >> del;
        if (del != MSG_END) return;
        handle_CL_RECRUIT(*user, username);
        break;
      }

      case CL_TAKE_TALENT: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto talentName = Talent::Name{buffer};
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

      case CL_CAST: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto spellID = std::string{buffer};
        iss >> del;
        if (del != MSG_END) return;
        if (user->isStunned()) {
          sendMessage(client, WARNING_STUNNED);
          break;
        }
        handle_CL_CAST(*user, spellID);
        break;
      }

      case CL_ACCEPT_QUEST: {
        iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
        auto questID = Quest::ID{buffer};
        iss >> del;

        auto startSerial = size_t{};
        iss >> startSerial >> del;

        if (del != MSG_END) return;

        handle_CL_ACCEPT_QUEST(*user, questID, startSerial);
        break;
      }

      case CL_COMPLETE_QUEST: {
        iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
        auto questID = Quest::ID{buffer};
        iss >> del;

        auto endSerial = size_t{};
        iss >> endSerial >> del;

        if (del != MSG_END) return;

        handle_CL_COMPLETE_QUEST(*user, questID, endSerial);
        break;
      }

      case CL_ABANDON_QUEST: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        auto questID = Quest::ID{buffer};
        iss >> del;

        if (del != MSG_END) return;

        handle_CL_ABANDON_QUEST(*user, questID);
        break;
      }

      case CL_SAY: {
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        std::string message(buffer);
        iss >> del;
        if (del != MSG_END) return;
        broadcast(SV_SAY, makeArgs(user->name(), message));
        break;
      }

      case CL_WHISPER: {
        iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
        std::string username(buffer);
        iss >> del;
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        std::string message(buffer);
        iss >> del;
        if (del != MSG_END) return;
        auto it = _usersByName.find(username);
        if (it == _usersByName.end()) {
          sendMessage(client, ERROR_INVALID_USER);
          break;
        }
        const User *target = it->second;
        sendMessage(target->socket(), SV_WHISPER,
                    makeArgs(user->name(), message));
        break;
      }

      case DG_GIVE: {
        if (!isDebug()) break;
        iss.get(buffer, BUFFER_SIZE, MSG_END);
        std::string id(buffer);
        iss >> del;
        if (del != MSG_END) return;
        const auto it = _items.find(id);
        if (it == _items.end()) {
          sendMessage(client, ERROR_INVALID_ITEM);
          break;
        }
        const ServerItem &item = *it;
        ;
        user->giveItem(&item, item.stackSize());
        break;
      }

      case DG_UNLOCK: {
        if (!isDebug()) break;
        if (del != MSG_END) return;
        ProgressLock::unlockAll(*user);
        break;
      }

      case DG_LEVEL: {
        if (!isDebug()) break;
        if (del != MSG_END) return;
        auto remainingXP = User::XP_PER_LEVEL[user->level()] - user->xp();
        user->addXP(remainingXP);
        break;
      }

      case DG_TELEPORT: {
        double x, y;
        iss >> x >> del >> y >> del;
        if (del != MSG_END) return;

        auto oldLoc = user->location();
        auto newLoc = MapPoint{x, y};

        user->location(newLoc);

        broadcastToArea(oldLoc, SV_LOCATION_INSTANT,
                        makeArgs(user->name(), newLoc.x, newLoc.y));
        broadcastToArea(newLoc, SV_LOCATION_INSTANT,
                        makeArgs(user->name(), newLoc.x, newLoc.y));
        sendRelevantEntitiesToUser(*user);
      }

      default:
        _debug << Color::TODO << "Unhandled message: " << msg << Log::endl;
    }
  }
}

void Server::handle_CL_START_WATCHING(User &user, size_t serial) {
  Entity *ent = _entities.find(serial);
  if (!isEntityInRange(user.socket(), user, ent, true)) return;

  ent->describeSelfToNewWatcher(user);
  ent->addWatcher(user.name());
}

void Server::handle_CL_LOGIN_EXISTING(const Socket &client,
                                      const std::string &name,
                                      const std::string &clientVersion) {
#ifndef _DEBUG
  // Check that version matches
  if (clientVersion != version()) {
    sendMessage(client, WARNING_WRONG_VERSION, version());
    return;
  }
#endif

  // Check that username is valid
  if (!isUsernameValid(name)) {
    sendMessage(client, WARNING_INVALID_USERNAME);
    return;
  }

  // Check that user isn't already logged in
  if (_usersByName.find(name) != _usersByName.end()) {
    sendMessage(client, WARNING_DUPLICATE_USERNAME);
    return;
  }

  // Check that user exists
  auto userFile = _userFilesPath + name + ".usr";
  if (!fileExists(userFile)) {
#ifndef _DEBUG
    sendMessage(client, WARNING_USER_DOESNT_EXIST);
    return;
#else
    addUser(client, name, _classes.begin()->first);
    return;
#endif
  }

  addUser(client, name);
}

void Server::handle_CL_LOGIN_NEW(const Socket &client, const std::string &name,
                                 const std::string &classID,
                                 std::string &clientVersion) {
#ifndef _DEBUG
  // Check that version matches
  if (clientVersion != version()) {
    sendMessage(client, WARNING_WRONG_VERSION, version());
    return;
  }
#endif

  // Check that username is valid
  if (!isUsernameValid(name)) {
    sendMessage(client, WARNING_INVALID_USERNAME);
    return;
  }

  // Check that user doesn't exist
  auto userFile = _userFilesPath + name + ".usr";
  if (fileExists(userFile)) {
    sendMessage(client, WARNING_NAME_TAKEN);
    return;
  }

  addUser(client, name, classID);
}

void Server::handle_CL_TAKE_ITEM(User &user, size_t serial, size_t slotNum) {
  if (serial == INVENTORY) {
    sendMessage(user.socket(), ERROR_TAKE_SELF);
    return;
  }

  Entity *pEnt = nullptr;
  ServerItem::Slot *pSlot;
  if (serial == GEAR)
    pSlot = user.getSlotToTakeFromAndSendErrors(slotNum, user);
  else {
    pEnt = _entities.find(serial);
    if (!pEnt) {
      sendMessage(user.socket(), WARNING_DOESNT_EXIST);
      return;
    }
    pSlot = pEnt->getSlotToTakeFromAndSendErrors(slotNum, user);
  }
  if (pSlot == nullptr) return;
  ServerItem::Slot &slot = *pSlot;

  // Attempt to give item to user
  size_t remainder = user.giveItem(slot.first, slot.second);
  if (remainder > 0) {
    slot.second = remainder;
    sendMessage(user.socket(), WARNING_INVENTORY_FULL);
  } else {
    slot.first = nullptr;
    slot.second = 0;
  }

  if (serial ==
      GEAR) {  // Tell user about his empty gear slot, and updated stats
    sendInventoryMessage(user, slotNum, GEAR);
    user.updateStats();

  } else {  // Alert object's watchers
    for (auto username : pEnt->watchers())
      pEnt->alertWatcherOnInventoryChange(*_usersByName[username], slotNum);
  }
}

void Server::handle_CL_LEAVE_CITY(User &user) {
  auto city = _cities.getPlayerCity(user.name());
  if (city.empty()) {
    sendMessage(user.socket(), ERROR_NOT_IN_CITY);
    return;
  }
  if (_kings.isPlayerAKing(user.name())) {
    sendMessage(user.socket(), ERROR_KING_CANNOT_LEAVE_CITY);
    return;
  }
  _cities.removeUserFromCity(user, city);
}

void Server::handle_CL_CEDE(User &user, size_t serial) {
  if (serial == INVENTORY || serial == GEAR) {
    sendMessage(user.socket(), WARNING_DOESNT_EXIST);
    return;
  }
  auto *obj = _entities.find<Object>(serial);

  if (!obj->permissions().isOwnedByPlayer(user.name())) {
    sendMessage(user.socket(), WARNING_NO_PERMISSION);
    return;
  }

  const City::Name &city = _cities.getPlayerCity(user.name());
  if (city.empty()) {
    sendMessage(user.socket(), ERROR_NOT_IN_CITY);
    return;
  }

  if (obj->objType().isPlayerUnique()) {
    sendMessage(user.socket(), ERROR_CANNOT_CEDE);
    return;
  }

  obj->permissions().setCityOwner(city);
}

void Server::handle_CL_GRANT(User &user, size_t serial,
                             const std::string &username) {
  auto *obj = _entities.find<Object>(serial);
  auto playerCity = cities().getPlayerCity(user.name());
  if (!obj->permissions().isOwnedByCity(playerCity)) {
    sendMessage(user.socket(), WARNING_NO_PERMISSION);
    return;
  }
  if (!_kings.isPlayerAKing(user.name())) {
    sendMessage(user.socket(), ERROR_NOT_A_KING);
    return;
  }
  obj->permissions().setPlayerOwner(username);
}

void Server::handle_CL_PERFORM_OBJECT_ACTION(User &user, size_t serial,
                                             const std::string &textArg) {
  if (serial == INVENTORY || serial == GEAR) {
    sendMessage(user.socket(), WARNING_DOESNT_EXIST);
    return;
  }
  auto *obj = _entities.find<Object>(serial);

  if (!obj->permissions().doesUserHaveAccess(user.name())) {
    sendMessage(user.socket(), WARNING_NO_PERMISSION);
    return;
  }

  const auto &objType = obj->objType();
  if (!objType.hasAction()) {
    sendMessage(user.socket(), ERROR_NO_ACTION);
    return;
  }

  if (objType.action().cost) {
    auto cost = ItemSet{};
    cost.add(objType.action().cost);
    if (!user.hasItems(cost)) {
      sendMessage(user.socket(), WARNING_ITEM_NEEDED);
      return;
    }
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

void Server::handle_CL_TARGET_ENTITY(User &user, size_t serial) {
  user.cancelAction();
  if (serial == INVENTORY || serial == GEAR) {
    user.setTargetAndAttack(nullptr);
    return;
  }

  auto target = _entities.find(serial);
  if (target == nullptr) {
    user.setTargetAndAttack(nullptr);
    sendMessage(user.socket(), WARNING_DOESNT_EXIST);
    return;
  }

  if (target->health() == 0) {
    user.setTargetAndAttack(nullptr);
    sendMessage(user.socket(), ERROR_TARGET_DEAD);
    return;
  }

  if (!target->canBeAttackedBy(user)) {
    user.setTargetAndAttack(nullptr);
    sendMessage(user.socket(), ERROR_ATTACKED_PEACFUL_PLAYER);
    return;
  }

  user.setTargetAndAttack(target);
}

void Server::handle_CL_TARGET_PLAYER(User &user,
                                     const std::string &targetUsername) {
  user.cancelAction();

  auto it = _usersByName.find(targetUsername);
  if (it == _usersByName.end()) {
    sendMessage(user.socket(), ERROR_INVALID_USER);
    return;
  }
  User *targetUser = const_cast<User *>(it->second);
  if (targetUser->health() == 0) {
    user.setTargetAndAttack(nullptr);
    sendMessage(user.socket(), ERROR_TARGET_DEAD);
    return;
  }
  if (!_wars.isAtWar(user.name(), targetUsername)) {
    user.setTargetAndAttack(nullptr);
    sendMessage(user.socket(), ERROR_ATTACKED_PEACFUL_PLAYER);
    return;
  }

  user.setTargetAndAttack(targetUser);
}

void Server::handle_CL_SELECT_ENTITY(User &user, size_t serial) {
  user.cancelAction();
  user.action(User::NO_ACTION);

  if (serial == INVENTORY || serial == GEAR) {
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
}

void Server::handle_CL_SELECT_PLAYER(User &user,
                                     const std::string &targetUsername) {
  user.cancelAction();

  auto it = _usersByName.find(targetUsername);
  if (it == _usersByName.end()) {
    sendMessage(user.socket(), ERROR_INVALID_USER);
    return;
  }
  User *targetUser = const_cast<User *>(it->second);
  user.target(targetUser);
}

void Server::handle_CL_RECRUIT(User &user, const std::string &username) {
  const auto &cityName = _cities.getPlayerCity(user.name());
  if (cityName.empty()) {
    sendMessage(user.socket(), ERROR_NOT_IN_CITY);
    return;
  }
  if (!_cities.getPlayerCity(username).empty()) {
    sendMessage(user.socket(), ERROR_ALREADY_IN_CITY);
    return;
  }
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
    if (!_cities.isPlayerInACity(user.name())) {
      sendMessage(user.socket(), ERROR_NOT_IN_CITY);
      return;
    }
    if (!_kings.isPlayerAKing(user.name())) {
      sendMessage(user.socket(), ERROR_NOT_A_KING);
      return;
    }
  }

  _wars.sueForPeace(proposer, enemy);

  // Alert the proposer
  if (proposer.type == Belligerent::PLAYER)
    sendMessage(user.socket(), codeForProposer, name);
  else
    broadcastToCity(proposer.name, codeForProposer, name);

  // Alert the enemy
  if (enemy.type == Belligerent::PLAYER)
    sendMessageIfOnline(enemy.name, codeForEnemy, proposer.name);
  else
    broadcastToCity(enemy.name, codeForEnemy, proposer.name);
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
    if (!_cities.isPlayerInACity(user.name())) {
      sendMessage(user.socket(), ERROR_NOT_IN_CITY);
      return;
    }
    if (!_kings.isPlayerAKing(user.name())) {
      sendMessage(user.socket(), ERROR_NOT_A_KING);
      return;
    }
  }

  auto canceled = _wars.cancelPeaceOffer(proposer, enemy);
  if (!canceled) return;

  // Alert the proposer
  if (proposer.type == Belligerent::PLAYER)
    sendMessage(user.socket(), codeForProposer, name);
  else
    broadcastToCity(proposer.name, codeForProposer, name);

  // Alert the enemy
  if (enemy.type == Belligerent::PLAYER)
    sendMessageIfOnline(enemy.name, codeForEnemy, proposer.name);
  else
    broadcastToCity(enemy.name, codeForEnemy, proposer.name);
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
    sendMessage(user.socket(), codeForProposer, name);
  else
    broadcastToCity(proposer.name, codeForProposer, name);

  // Alert the enemy
  if (enemy.type == Belligerent::PLAYER)
    sendMessageIfOnline(enemy.name, codeForEnemy, proposer.name);
  else
    broadcastToCity(enemy.name, codeForEnemy, proposer.name);
}

void Server::handle_CL_TAKE_TALENT(User &user, const Talent::Name &talentName) {
  auto &userClass = user.getClass();
  const auto &classType = userClass.type();
  auto talent = classType.findTalent(talentName);
  if (talent == nullptr) {
    sendMessage(user.socket(), ERROR_INVALID_TALENT);
    return;
  }
  if (!user.getClass().canTakeATalent()) {
    sendMessage(user.socket(), WARNING_NO_TALENT_POINTS);
    return;
  }

  auto &tier = talent->tier();

#ifndef _DEBUG
  if (tier.reqPointsInTree > 0 &&
      user.getClass().pointsInTree(talent->tree()) < tier.reqPointsInTree) {
    sendMessage(user.socket(), WARNING_MISSING_REQ_FOR_TALENT);
    return;
  }

  if (tier.hasItemCost()) {
    if (!user.hasItems(tier.costTag, tier.costQuantity)) {
      sendMessage(user.socket(), WARNING_MISSING_ITEMS_FOR_TALENT);
      return;
    }
    user.removeItems(tier.costTag, tier.costQuantity);
  }
#endif

  if (talent->type() == Talent::SPELL && userClass.hasTalent(talent)) {
    sendMessage(user.socket(), ERROR_ALREADY_KNOW_SPELL);
    return;
  }

  userClass.takeTalent(talent);

  switch (talent->type()) {
    case Talent::SPELL:
      sendMessage(user.socket(), SV_LEARNED_SPELL, talent->spellID());
      break;

    case Talent::STATS:
      user.updateStats();
      break;
  }
}

void Server::handle_CL_UNLEARN_TALENTS(User &user) {
  user.getClass().unlearnAll();
}

CombatResult Server::handle_CL_CAST(User &user, const std::string &spellID,
                                    bool castingFromItem) {
  auto it = _spells.find(spellID);
  if (it == _spells.end()) return FAIL;
  const auto &spell = *it->second;

  // Learned-spell check
  if (!castingFromItem && !user.getClass().knowsSpell(spellID)) {
    sendMessage(user.socket(), ERROR_DONT_KNOW_SPELL);
    return FAIL;
  }

  // Energy check
  if (user.energy() < spell.cost()) {
    sendMessage(user.socket(), WARNING_NOT_ENOUGH_ENERGY);
    return FAIL;
  }

  if (user.isSpellCoolingDown(spellID)) return FAIL;

  auto outcome = user.castSpell(spell);

  user.onSpellcast(spellID, spell);

  return outcome;
}

void Server::handle_CL_ACCEPT_QUEST(User &user, const Quest::ID &questID,
                                    size_t startSerial) {
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

  if (quest->exclusiveToClass && user.getClass().type().id() != "politician")
    return;

  user.startQuest(*quest);
}

void Server::handle_CL_COMPLETE_QUEST(User &user, const Quest::ID &quest,
                                      size_t endSerial) {
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

void Server::broadcast(MessageCode msgCode, const std::string &args) {
  for (const User &user : _users) {
    sendMessage(user.socket(), msgCode, args);
  }
}

void Server::broadcastToArea(const MapPoint &location, MessageCode msgCode,
                             const std::string &args) const {
  for (const User *user : this->findUsersInArea(location)) {
    user->sendMessage(msgCode, args);
  }
}

void Server::broadcastToCity(const std::string &cityName, MessageCode msgCode,
                             const std::string &args) const {
  if (!_cities.doesCityExist(cityName)) {
    _debug << Color::TODO << "City " << cityName << " does not exist."
           << Log::endl;
    return;
  }

  for (const auto &citizen : _cities.membersOf(cityName)) {
    sendMessageIfOnline(citizen, msgCode, args);
  }
}

void Server::sendMessage(const Socket &dstSocket, MessageCode msgCode,
                         const std::string &args) const {
  auto message = compileMessage(msgCode, args);
  _socket.sendMessage(message, dstSocket);
}

void Server::sendMessageIfOnline(const std::string username,
                                 MessageCode msgCode,
                                 const std::string &args) const {
  auto it = _usersByName.find(username);
  if (it == _usersByName.end()) return;
  sendMessage(it->second->socket(), msgCode, args);
}

std::string Server::compileMessage(MessageCode msgCode,
                                   const std::string &args) {
  std::ostringstream oss;
  oss << MSG_START << msgCode;
  if (args != "") oss << MSG_DELIM << args;
  oss << MSG_END;

  return oss.str();
}

void Server::sendInventoryMessageInner(
    const User &user, size_t serial, size_t slot,
    const ServerItem::vect_t &itemVect) const {
  if (slot >= itemVect.size()) {
    sendMessage(user.socket(), ERROR_INVALID_SLOT);
    return;
  }
  const auto &containerSlot = itemVect[slot];
  std::string itemID = containerSlot.first ? containerSlot.first->id() : "none";
  sendMessage(user.socket(), SV_INVENTORY,
              makeArgs(serial, slot, itemID, containerSlot.second));
}

void Server::sendInventoryMessage(const User &user, size_t slot,
                                  const Object &obj) const {
  if (!obj.hasContainer()) {
    assert(false);
    return;
  }
  sendInventoryMessageInner(user, obj.serial(), slot, obj.container().raw());
}

// Special serials only
void Server::sendInventoryMessage(const User &user, size_t slot,
                                  size_t serial) const {
  const ServerItem::vect_t *container = nullptr;
  switch (serial) {
    case INVENTORY:
      container = &user.inventory();
      break;
    case GEAR:
      container = &user.gear();
      break;
    default:
      assert(false);
  }
  sendInventoryMessageInner(user, serial, slot, *container);
}

void Server::sendMerchantSlotMessage(const User &user, const Object &obj,
                                     size_t slot) const {
  assert(slot < obj.merchantSlots().size());
  const MerchantSlot &mSlot = obj.merchantSlot(slot);
  if (mSlot)
    sendMessage(user.socket(), SV_MERCHANT_SLOT,
                makeArgs(obj.serial(), slot, mSlot.wareItem->id(),
                         mSlot.wareQty, mSlot.priceItem->id(), mSlot.priceQty));
  else
    sendMessage(user.socket(), SV_MERCHANT_SLOT,
                makeArgs(obj.serial(), slot, "", 0, "", 0));
}

void Server::sendConstructionMaterialsMessage(const User &user,
                                              const Object &obj) const {
  size_t n = obj.remainingMaterials().numTypes();
  std::string args = makeArgs(obj.serial(), n);
  for (const auto &pair : obj.remainingMaterials()) {
    args = makeArgs(args, pair.first->id(), pair.second);
  }
  sendMessage(user.socket(), SV_CONSTRUCTION_MATERIALS, args);
}

void Server::sendNewBuildsMessage(const User &user,
                                  const std::set<std::string> &ids) const {
  if (!ids.empty()) {  // New constructions unlocked!
    std::string args = makeArgs(ids.size());
    for (const std::string &id : ids) args = makeArgs(args, id);
    sendMessage(user.socket(), SV_NEW_CONSTRUCTIONS, args);
  }
}

void Server::sendNewRecipesMessage(const User &user,
                                   const std::set<std::string> &ids) const {
  if (!ids.empty()) {  // New recipes unlocked!
    std::string args = makeArgs(ids.size());
    for (const std::string &id : ids) args = makeArgs(args, id);
    sendMessage(user.socket(), SV_NEW_RECIPES, args);
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
  sendMessage(it->second->socket(), code, otherBelligerent.name);
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
  Permissions::Owner owner(Permissions::Owner::PLAYER, user.name());
  for (auto pEntity : _entities) {
    auto *pObject = dynamic_cast<const Object *>(pEntity);

    bool notAnObject = pObject == nullptr;
    if (notAnObject) continue;

    bool newUserOwnsThisObject =
        _objectsByOwner.isObjectOwnedBy(pObject->serial(), owner);
    if (newUserOwnsThisObject) {
      entitiesToDescribe.insert(pEntity);
      if (!pObject->isDead()) user.onNewOwnedObject(pObject->objType());
    }
  }

  // Send
  for (const Entity *entity : entitiesToDescribe) {
    if (entity->type() == nullptr) {
      _debug("Null-type object skipped", Color::TODO);
      continue;
    }
    entity->sendInfoToClient(user);
  }
}
