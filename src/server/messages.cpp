#include <cassert>

#include "ProgressLock.h"
#include "Server.h"
#include "Vehicle.h"
#include "objects/Deconstruction.h"
#include "../messageCodes.h"

void Server::handleMessage(const Socket &client, const std::string &msg){
    _debug(msg);
    int msgCode;
    char del;
    static char buffer[BUFFER_SIZE+1];
    std::istringstream iss(msg);
    User *user = nullptr;
    while (iss.peek() == MSG_START) {
        iss >> del >> msgCode >> del;
        
        // Discard message if this client has not yet sent CL_I_AM
        std::set<User>::iterator it = _users.find(client);
        if (it == _users.end() && msgCode != CL_I_AM) {
            continue;
        }
        if (msgCode != CL_I_AM) {
            User & userRef = const_cast<User&>(*it);
            user = &userRef;
            user->contact();
        }

        switch(msgCode) {

        case CL_PING:
        {
            ms_t timeSent;
            iss >> timeSent  >> del;
            if (del != MSG_END)
                return;
            sendMessage(user->socket(), SV_PING_REPLY, makeArgs(timeSent));
            break;
        }

        case CL_I_AM:
        {
            std::string name;
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            name = std::string(buffer);
            iss >> del;
            if (del != MSG_END)
                return;

            // Check that username is valid
            bool invalid = false;
            for (char c : name){
                if (c < 'a' || c > 'z') {
                    sendMessage(client, SV_INVALID_USERNAME);
                    invalid = true;
                    break;
                }
            }
            if (invalid)
                break;

            // Check that user isn't already logged in
            if (_usersByName.find(name) != _usersByName.end()) {
                sendMessage(client, SV_DUPLICATE_USERNAME);
                invalid = true;
                break;
            }

            addUser(client, name);

            break;
        }

        case CL_LOCATION:
        {
            double x, y;
            iss >> x >> del >> y >> del;
            if (del != MSG_END)
                return;
            if (user->action() != User::ATTACK)
                user->cancelAction();
            if (user->isDriving()){
                // Move vehicle and user together
                size_t vehicleSerial = user->driving();
                Vehicle &vehicle = * _entities.find<Vehicle>(vehicleSerial);
                vehicle.updateLocation(Point(x, y));
                user->updateLocation(vehicle.location());
                break;
            }
            user->updateLocation(Point(x, y));
            break;
        }

        case CL_CRAFT:
        {
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            std::string id(buffer);
            iss >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();
            const std::set<Recipe>::const_iterator it = _recipes.find(id);
            if (!user->knowsRecipe(id)){
                sendMessage(client, SV_UNKNOWN_RECIPE);
                break;
            }
            ItemSet remaining;
            if (!user->hasItems(it->materials())) {
                sendMessage(client, SV_NEED_MATERIALS);
                break;
            }
            if (!user->hasTools(it->tools())) {
                sendMessage(client, SV_NEED_TOOLS);
                break;
            }
            user->beginCrafting(*it);
            sendMessage(client, SV_ACTION_STARTED, makeArgs(it->time()));
            break;
        }

        case CL_CONSTRUCT:
        {
            double x, y;
            iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
            std::string id(buffer);
            iss >> del >> x >> del >> y >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();
            const ObjectType *objType = findObjectTypeByName(id);
            if (objType == nullptr){
                sendMessage(client, SV_INVALID_OBJECT);
                break;
            }
            if (!user->knowsConstruction(id)){
                sendMessage(client, SV_UNKNOWN_CONSTRUCTION);
                break;
            }
            if (objType->isUnique() && objType->numInWorld() == 1){
                sendMessage(client, SV_UNIQUE_OBJECT);
                break;
            }
            if (objType->isPlayerUnique() && user->hasPlayerUnique(objType->playerUniqueCategory())) {
                sendMessage(client, SV_PLAYER_UNIQUE_OBJECT, objType->playerUniqueCategory());
                break;
            }
            if (objType->isUnbuildable()){
                sendMessage(client, SV_UNBUILDABLE);
                break;
            }
            bool requiresTool = ! objType->constructionReq().empty();
            if (requiresTool && ! user->hasTool(objType->constructionReq())){
                sendMessage(client, SV_NEED_TOOLS);
                break;
            }
            const Point location(x, y);
            if (distance(user->collisionRect(), objType->collisionRect() + location) >
                ACTION_DISTANCE) {
                sendMessage(client, SV_TOO_FAR);
                break;
            }
            if (!isLocationValid(location, *objType)) {
                sendMessage(client, SV_BLOCKED);
                break;
            }
            user->beginConstructing(*objType, location);
            sendMessage(client, SV_ACTION_STARTED,
                        makeArgs(objType->constructionTime()));
            break;
        }

        case CL_CONSTRUCT_ITEM:
        {
            size_t slot;
            double x, y;
            iss >> slot >> del >> x >> del >> y >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();
            if (slot >= User::INVENTORY_SIZE) {
                sendMessage(client, SV_INVALID_SLOT);
                break;
            }
            const std::pair<const ServerItem *, size_t> &invSlot = user->inventory(slot);
            if (invSlot.first == nullptr) {
                sendMessage(client, SV_EMPTY_SLOT);
                break;
            }
            const ServerItem &item = *invSlot.first;
            if (item.constructsObject() == nullptr) {
                sendMessage(client, SV_CANNOT_CONSTRUCT);
                break;
            }
            const Point location(x, y);
            const ObjectType &objType = *item.constructsObject();
            if (distance(user->collisionRect(), objType.collisionRect() + location) >
                ACTION_DISTANCE) {
                sendMessage(client, SV_TOO_FAR);
                break;
            }
            if (!isLocationValid(location, objType)) {
                sendMessage(client, SV_BLOCKED);
                break;
            }
            const std::string constructionReq = objType.constructionReq();
            if (!(constructionReq.empty() || user->hasTool(constructionReq))) {
                sendMessage(client, SV_ITEM_NEEDED, constructionReq);
                break;
            }
            user->beginConstructing(objType, location, slot);
            sendMessage(client, SV_ACTION_STARTED,
                        makeArgs(objType.constructionTime()));
            break;
        }

        case CL_CANCEL_ACTION:
        {
            iss >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();
            break;
        }

        case CL_GATHER:
        {
            int serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();
            Object *obj = _entities.find<Object>(serial);
            if (!isEntityInRange(client, *user, obj)) {
                sendMessage(client, SV_DOESNT_EXIST);
                break;
            }
            assert (obj->type() != nullptr);
            // Check that the user meets the requirements
            if (obj->isBeingBuilt()){
                sendMessage(client, SV_UNDER_CONSTRUCTION);
                break;
            }
            if (!obj->permissions().doesUserHaveAccess(user->name())){
                sendMessage(client, SV_NO_PERMISSION);
                break;
            }
            const std::string &gatherReq = obj->objType().gatherReq();
            if (gatherReq != "none" && !user->hasTool(gatherReq)) {
                sendMessage(client, SV_ITEM_NEEDED, gatherReq);
                break;
            }
            // Check that it has an inventory
            if (obj->hasContainer() && !obj->container().isEmpty()){
                sendMessage(client, SV_NOT_EMPTY);
                break;
            }
            user->beginGathering(obj);
            sendMessage(client, SV_ACTION_STARTED, makeArgs(obj->objType().gatherTime()));
            break;
        }

        case CL_DECONSTRUCT:
        {
            int serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();
            Object *obj = _entities.find<Object>(serial);
            if (!isEntityInRange(client, *user, obj)) {
                sendMessage(client, SV_TOO_FAR);
                break;
            }
            if (obj->isBeingBuilt()){
                sendMessage(client, SV_UNDER_CONSTRUCTION);
                break;
            }
            assert (obj->type());
            if (!obj->permissions().doesUserHaveAccess(user->name())){
                sendMessage(client, SV_NO_PERMISSION);
                break;
            }
            // Check that the object can be deconstructed
            if (! obj->hasDeconstruction()){
                sendMessage(client, SV_CANNOT_DECONSTRUCT);
                break;
            }
            if (obj->health() < obj->maxHealth()){
                sendMessage(client, SV_DAMAGED_OBJECT);
                break;
            }
            if (!obj->isAbleToDeconstruct(*user)){
                break;
            }
            // Check that it isn't an occupied vehicle
            if (obj->classTag() == 'v' && !dynamic_cast<const Vehicle *>(obj)->driver().empty()){
                sendMessage(client, SV_VEHICLE_OCCUPIED);
                break;
            }

            user->beginDeconstructing(*obj);
            sendMessage(client, SV_ACTION_STARTED, makeArgs(
                    obj->deconstruction().timeToDeconstruct()));
            break;
        }

        case CL_DROP:
        {
            size_t serial, slot;
            iss >> serial >> del >> slot >> del;
            if (del != MSG_END)
                return;
            ServerItem::vect_t *container;
            Object *pObj = nullptr;
            bool breakMsg = false;
            switch(serial){
                case INVENTORY: container = &user->inventory(); break;
                case GEAR:      container = &user->gear();      break;
                default:
                    pObj = _entities.find<Object>(serial);
                    if (!pObj->hasContainer()){
                        sendMessage(client, SV_NO_INVENTORY);
                        breakMsg = true;
                        break;
                    }
                    if (!isEntityInRange(client, *user, pObj)){
                        breakMsg = true;
                        break;
                    }
                    container = &pObj->container().raw();
            }
            if (breakMsg)
                break;

            if (slot >= container->size()) {
                sendMessage(client, SV_INVALID_SLOT);
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

        case CL_SWAP_ITEMS:
        {
            size_t obj1, slot1, obj2, slot2;
            iss >> obj1 >> del
                >> slot1 >> del
                >> obj2 >> del
                >> slot2 >> del;
            if (del != MSG_END)
                return;
            ServerItem::vect_t
                *containerFrom = nullptr,
                *containerTo = nullptr;
            Object
                *pObj1 = nullptr,
                *pObj2 = nullptr;
            bool breakMsg = false;
            switch (obj1){
                case INVENTORY: containerFrom = &user->inventory(); break;
                case GEAR:      containerFrom = &user->gear();      break;
                default:
                    pObj1 = _entities.find<Object>(obj1);
                    if (!pObj1->hasContainer()){
                        sendMessage(client, SV_NO_INVENTORY);
                        breakMsg = true;
                        break;
                    }
                    if (!isEntityInRange(client, *user, pObj1)){
                        sendMessage(client, SV_TOO_FAR);
                        breakMsg = true;
                    }
                    if (!pObj1->permissions().doesUserHaveAccess(user->name())){
                        sendMessage(client, SV_NO_PERMISSION);
                        breakMsg = true;
                    }
                    containerFrom = &pObj1->container().raw();
            }
            if (breakMsg)
                break;
            bool isConstructionMaterial = false;
            switch (obj2){
                case INVENTORY: containerTo = &user->inventory(); break;
                case GEAR:      containerTo = &user->gear();      break;
                default:
                    pObj2 = _entities.find<Object>(obj2);
                    if (pObj2 != nullptr &&
                        pObj2->isBeingBuilt() &&
                        slot2 == 0)
                            isConstructionMaterial = true;
                    if (!isConstructionMaterial && !pObj2->hasContainer()){
                        sendMessage(client, SV_NO_INVENTORY);
                        breakMsg = true;
                        break;
                    }
                    if (!isEntityInRange(client, *user, pObj2)){
                        sendMessage(client, SV_TOO_FAR);
                        breakMsg = true;
                    }
                    if (!pObj2->permissions().doesUserHaveAccess(user->name())){
                        sendMessage(client, SV_NO_PERMISSION);
                        breakMsg = true;
                    }
                    containerTo = &pObj2->container().raw();
            }
            if (breakMsg)
                break;

            if (slot1 >= containerFrom->size() ||
                !isConstructionMaterial && slot2 >= containerTo->size() ||
                isConstructionMaterial && slot2 > 0) {
                sendMessage(client, SV_INVALID_SLOT);
                break;
            }

            auto &slotFrom = (*containerFrom)[slot1];
            assert(slotFrom.first != nullptr);

            if (isConstructionMaterial){
                size_t
                    qtyInSlot = slotFrom.second,
                    qtyNeeded = pObj2->remainingMaterials()[slotFrom.first],
                    qtyToTake = min(qtyInSlot, qtyNeeded);

                if (qtyNeeded == 0){
                    sendMessage(client, SV_WRONG_MATERIAL);
                    break;
                }

                // Remove from object requirements
                pObj2->remainingMaterials().remove(slotFrom.first, qtyToTake);
                for (auto username : pObj2->watchers())
                    if (pObj2->permissions().doesUserHaveAccess(username))
                        sendConstructionMaterialsMessage(*_usersByName[username], *pObj2);

                // Remove items from user
                slotFrom.second -= qtyToTake;
                if (slotFrom.second == 0)
                    slotFrom.first = nullptr;
                sendInventoryMessage(*user, slot1, obj1);

                // Check if this action completed construction
                if (!pObj2->isBeingBuilt()){

                    // Send to all nearby players, since object appearance will change
                    for (const User *otherUser : findUsersInArea(user->location())){
                        if (otherUser == user)
                            continue;
                        sendConstructionMaterialsMessage(*otherUser, *pObj2);
                    }
                    
                    // Trigger completing user's unlocks
                    if (user->knowsConstruction(pObj2->type()->id()))
                        ProgressLock::triggerUnlocks(
                                *user, ProgressLock::CONSTRUCTION, pObj2->type());
                }

                break;
            }

            auto &slotTo = (*containerTo)[slot2];
            
            if (pObj1 != nullptr && pObj1->classTag() == 'n' && slotTo.first != nullptr ||
                pObj2 != nullptr && pObj2->classTag() == 'n' && slotFrom.first != nullptr){
                    sendMessage(client, SV_NPC_SWAP);
                    break;
            }

            // Check gear-slot compatibility
            if (obj1 == GEAR && slotTo.first != nullptr && slotTo.first->gearSlot() != slot1 ||
                obj2 == GEAR && slotFrom.first != nullptr && slotFrom.first->gearSlot() != slot2){
                    sendMessage(client, SV_NOT_GEAR);
                    break;
            }

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
                    if (slotFrom.first != nullptr)
                        gearID = slotFrom.first->id();
                } else {
                    gearSlot = slot2;
                    if (slotTo.first != nullptr)
                        gearID = slotTo.first->id();
                }
                for (const User *otherUser : findUsersInArea(user->location())){
                    if (otherUser == user)
                        continue;
                    sendMessage(otherUser->socket(), SV_GEAR, makeArgs(
                            user->name(), gearSlot, gearID));
                }
            }

            // Alert relevant users
            if (obj1 == INVENTORY || obj1 == GEAR)
                sendInventoryMessage(*user, slot1, obj1);
            else
                for (auto username : pObj1->watchers())
                    if (pObj1->permissions().doesUserHaveAccess(username))
                        sendInventoryMessage(*_usersByName[username], slot1, *pObj1);
            if (obj2 == INVENTORY || obj2 == GEAR){
                sendInventoryMessage(*user, slot2, obj2);
                ProgressLock::triggerUnlocks(*user, ProgressLock::ITEM, slotTo.first);
            } else
                for (auto username : pObj2->watchers())
                    if (pObj2->permissions().doesUserHaveAccess(username))
                        sendInventoryMessage(*_usersByName[username], slot2, *pObj2);

            break;
        }

        case CL_TAKE_ITEM:
        {
            size_t serial, slotNum;
            iss >> serial >> del
                >> slotNum >> del;
            if (del != MSG_END)
                return;

            handle_CL_TAKE_ITEM(*user, serial, slotNum);
            break;
        }

        case CL_TRADE:
        {
            size_t serial, slot;
            iss >> serial >> del >> slot >> del;
            if (del != MSG_END)
                return;

            // Check that merchant slot is valid
            Object *obj = _entities.find<Object>(serial);
            if (!isEntityInRange(client, *user, obj))
                break;
            if (obj->isBeingBuilt()){
                sendMessage(client, SV_UNDER_CONSTRUCTION);
                break;
            }
            size_t slots = obj->objType().merchantSlots();
            if (slots == 0){
                sendMessage(client, SV_NOT_MERCHANT);
                break;
            } else if (slot >= slots){
                sendMessage(client, SV_INVALID_MERCHANT_SLOT);
                break;
            }
            const MerchantSlot &mSlot = obj->merchantSlot(slot);
            if (!mSlot){
                sendMessage(client, SV_INVALID_MERCHANT_SLOT);
                break;
            }

            // Check that user has price
            if (mSlot.price() > user->inventory()){
                sendMessage(client, SV_NO_PRICE);
                break;
            }

            // Check that user has inventory space
            const ServerItem *priceItem = toServerItem(mSlot.priceItem);
            if (!obj->hasContainer()){
                sendMessage(client, SV_NO_INVENTORY);
                break;
            }
            if (!vectHasSpace(user->inventory(), priceItem, mSlot.wareQty)){
                sendMessage(client, SV_INVENTORY_FULL);
                break;
            }

            bool bottomless = obj->objType().bottomlessMerchant();
            if (!bottomless){
                // Check that object has items in stock
                if (mSlot.ware() > obj->container().raw()){
                    sendMessage(client, SV_NO_WARE);
                    break;
                }

                // Check that object has inventory space
                if (!vectHasSpace(obj->container().raw(), priceItem, mSlot.priceQty)){
                    sendMessage(client, SV_MERCHANT_INVENTORY_FULL);
                    break;
                }
            }

            // Take price from user
            user->removeItems(mSlot.price());

            if  (!bottomless){
                // Take ware from object
                obj->container().removeItems(mSlot.ware());

                // Give price to object
                obj->container().addItems(toServerItem(mSlot.priceItem), mSlot.priceQty);
            }

            // Give ware to user
            const ServerItem *wareItem = toServerItem(mSlot.wareItem);
            user->giveItem(wareItem, mSlot.wareQty);

            break;
        }

        case CL_SET_MERCHANT_SLOT:
        {
            size_t serial, slot, wareQty, priceQty;
            iss >> serial >> del >> slot >> del;
            iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
            std::string ware(buffer);
            iss >> del >> wareQty >> del;
            iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
            std::string price(buffer);
            iss >> del >> priceQty >> del;
            if (del != MSG_END)
                return;
            Object *obj = _entities.find<Object>(serial);
            if (!isEntityInRange(client, *user, obj))
                break;
            if (obj->isBeingBuilt()){
                sendMessage(client, SV_UNDER_CONSTRUCTION);
                break;
            }
            if (!obj->permissions().doesUserHaveAccess(user->name())){
                sendMessage(client, SV_NO_PERMISSION);
                break;
            }
            size_t slots = obj->objType().merchantSlots();
            if (slots == 0){
                sendMessage(client, SV_NOT_MERCHANT);
                break;
            }
            if (slot >= slots){
                sendMessage(client, SV_INVALID_MERCHANT_SLOT);
                break;
            }
            auto wareIt = _items.find(ware);
            if (wareIt == _items.end()){
                sendMessage(client, SV_INVALID_ITEM);
                break;
            }
            auto priceIt = _items.find(price);
            if (priceIt == _items.end()){
                sendMessage(client, SV_INVALID_ITEM);
                break;
            }
            MerchantSlot &mSlot = obj->merchantSlot(slot);
            mSlot = MerchantSlot(&*wareIt, wareQty, &*priceIt, priceQty);

            // Alert watchers
            for (auto username : obj->watchers())
                sendMerchantSlotMessage(*_usersByName[username], *obj, slot);
            break;
        }

        case CL_CLEAR_MERCHANT_SLOT:
        {
            size_t serial, slot;
            iss >> serial >> del >> slot >> del;
            if (del != MSG_END)
                return;
            Object *obj = _entities.find<Object>(serial);
            if (!isEntityInRange(client, *user, obj))
                break;
            if (obj->isBeingBuilt()){
                sendMessage(client, SV_UNDER_CONSTRUCTION);
                break;
            }
            if (!obj->permissions().doesUserHaveAccess(user->name())){
                sendMessage(client, SV_NO_PERMISSION);
                break;
            }
            size_t slots = obj->objType().merchantSlots();
            if (slots == 0){
                sendMessage(client, SV_NOT_MERCHANT);
                break;
            }
            if (slot >= slots){
                sendMessage(client, SV_INVALID_MERCHANT_SLOT);
                break;
            }
            obj->merchantSlot(slot) = MerchantSlot();

            // Alert watchers
            for (auto username : obj->watchers())
                sendMerchantSlotMessage(*_usersByName[username], *obj, slot);
            break;
        }

        case CL_MOUNT:
        {
            size_t serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            Object *obj = _entities.find<Object>(serial);
            if (!isEntityInRange(client, *user, obj))
                break;
            if (obj->isBeingBuilt()){
                sendMessage(client, SV_UNDER_CONSTRUCTION);
                break;
            }
            if (!obj->permissions().doesUserHaveAccess(user->name())){
                sendMessage(client, SV_NO_PERMISSION);
                break;
            }
            if (obj->classTag() != 'v'){
                sendMessage(client, SV_NOT_VEHICLE);
                break;
            }
            Vehicle *v = dynamic_cast<Vehicle *>(obj);
            if (!v->driver().empty() && v->driver() != user->name()){
                sendMessage(client, SV_VEHICLE_OCCUPIED);
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

        case CL_DISMOUNT:
        {
            Point target;
            iss >> target.x >> del >> target.y >> del;
            if (del != MSG_END)
                return;
            if (!user->isDriving()){
                sendMessage(client, SV_NO_VEHICLE);
                break;
            }
            Rect dstRect = user->type()->collisionRect() + target;
            if (distance(dstRect, user->collisionRect()) > ACTION_DISTANCE){
                sendMessage(client, SV_TOO_FAR);
                break;
            }
            if (!isLocationValid(target, User::OBJECT_TYPE)){
                sendMessage(client, SV_BLOCKED);
                break;
            }
            size_t serial = user->driving();
            Object *obj = _entities.find<Object>(serial);
            Vehicle *v = dynamic_cast<Vehicle *>(obj);

            // Move him before dismounting him, to avoid unnecessary collision/distance checks
            user->updateLocation(target);

            v->driver("");
            user->driving(0);
            for (const User *u : findUsersInArea(user->location()))
                sendMessage(u->socket(), SV_UNMOUNTED, makeArgs(serial, user->name()));

            break;
        }

        case CL_START_WATCHING:
        {
            size_t serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            handle_CL_START_WATCHING(*user, serial);
            break;
        }

        case CL_STOP_WATCHING:
        {
            size_t serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            Object *obj = _entities.find<Object>(serial);
            if (obj == nullptr) {
                sendMessage(client, SV_DOESNT_EXIST);
                break;
            }

            obj->removeWatcher(user->name());

            break;
        }

        case CL_TARGET_ENTITY:
        {
            size_t serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();
            if (serial == INVENTORY || serial == GEAR){
                user->setTargetAndAttack(nullptr);
                break;
            }

            Entity *target = _entities.find(serial);
            if (target == nullptr) {
                user->setTargetAndAttack(nullptr);
                sendMessage(client, SV_DOESNT_EXIST);
                break;
            }

            if (target->health() == 0){
                user->setTargetAndAttack(nullptr);
                sendMessage(client, SV_TARGET_DEAD);
                break;
            }

            user->setTargetAndAttack(target);

            break;
        }

        case CL_TARGET_PLAYER:
        {
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            std::string targetUsername(buffer);
            iss >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();

            auto it = _usersByName.find(targetUsername);
            if (it == _usersByName.end()){
                sendMessage(client, SV_INVALID_USER);
                break;
            }
            User *targetUser = const_cast<User *>(it->second);
            if (targetUser->health() == 0){
                user->setTargetAndAttack(nullptr);
                sendMessage(client, SV_TARGET_DEAD);
                break;
            }
            if (! _wars.isAtWar(user->name(), targetUsername)){
                user->setTargetAndAttack(nullptr);
                sendMessage(client, SV_AT_PEACE);
                break;
            }

            user->setTargetAndAttack(targetUser);

            break;
        }

        case CL_DECLARE_WAR_ON_PLAYER:
        case CL_DECLARE_WAR_ON_CITY:
        {
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            std::string targetName(buffer);
            iss >> del;
            if (del != MSG_END)
                return;
            Wars::Belligerent::Type targetType = msgCode == CL_DECLARE_WAR_ON_PLAYER ?
                    Wars::Belligerent::PLAYER :
                    Wars::Belligerent::CITY;
            Wars::Belligerent target(targetName, targetType);
            if (_wars.isAtWar(user->name(), target)){
                sendMessage(client, SV_ALREADY_AT_WAR);
                break;
            }
            _wars.declare(user->name(), target);
            break;
        }

        case CL_LEAVE_CITY:
        {
            if (del != MSG_END)
                return;
            handle_CL_LEAVE_CITY(*user);
            break;
        }

        case CL_CEDE:
        {
            size_t serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            handle_CL_CEDE(*user, serial);
            break;
        }

        case CL_PERFORM_OBJECT_ACTION:
        {
            size_t serial;
            iss >> serial >> del;
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            auto textArg = std::string{ buffer };
            iss >> del;
            if (del != MSG_END)
                return;
            handle_CL_PERFORM_OBJECT_ACTION(*user, serial, textArg);
            break;
        }

        case CL_RECRUIT:
        {
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            auto username = std::string{ buffer };
            iss >> del;
            if (del != MSG_END)
                return;
            handle_CL_RECRUIT(*user, username);
            break;
        }

        case CL_SAY:
        {
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            std::string message(buffer);
            iss >> del;
            if (del != MSG_END)
                return;
            broadcast(SV_SAY, makeArgs(user->name(), message));
            break;
        }

        case CL_WHISPER:
        {
            iss.get(buffer, BUFFER_SIZE, MSG_DELIM);
            std::string username(buffer);
            iss >> del;
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            std::string message(buffer);
            iss >> del;
            if (del != MSG_END)
                return;
            auto it = _usersByName.find(username);
            if (it == _usersByName.end()) {
                sendMessage(client, SV_INVALID_USER);
                break;
            }
            const User *target = it->second;
            sendMessage(target->socket(), SV_WHISPER, makeArgs(user->name(), message));
            break;
        }

        case DG_GIVE:
        {
            if (!isDebug())
                break;
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            std::string id(buffer);
            iss >> del;
            if (del != MSG_END)
                return;
            const auto it = _items.find(id);
            if (it == _items.end()){
                sendMessage(client, SV_INVALID_ITEM);
                break;
            }
            const ServerItem &item = *it;;
            user->giveItem(&item, item.stackSize());
            break;
        }

        case DG_UNLOCK:
        {
            if (!isDebug())
                break;
            if (del != MSG_END)
                return;
            ProgressLock::unlockAll(*user);
            break;
        }

        default:
            _debug << Color::RED << "Unhandled message: " << msg << Log::endl;
        }
    }
}

void Server::handle_CL_START_WATCHING(User &user, size_t serial){
    Entity *ent = _entities.find(serial);
    if (!isEntityInRange(user.socket(), user, ent))
        return;

    ent->describeSelfToNewWatcher(user);
    ent->addWatcher(user.name());
}

void Server::handle_CL_TAKE_ITEM(User &user, size_t serial, size_t slotNum) {
    if (serial == INVENTORY) {
        sendMessage(user.socket(), SV_TAKE_SELF);
        return;
    }

    Entity *pEnt = nullptr;
    ServerItem::Slot *pSlot;
    if (serial == GEAR)
        pSlot = user.getSlotToTakeFromAndSendErrors(slotNum, user);
    else {
        pEnt = _entities.find(serial);
        pSlot = pEnt->getSlotToTakeFromAndSendErrors(slotNum, user);
    }
    if (pSlot == nullptr)
        return;
    ServerItem::Slot &slot = *pSlot;

    // Attempt to give item to user
    size_t remainder = user.giveItem(slot.first, slot.second);
    if (remainder > 0) {
        slot.second = remainder;
        sendMessage(user.socket(), SV_INVENTORY_FULL);
    } else {
        slot.first = nullptr;
        slot.second = 0;
    }

    if (serial == GEAR) { // Tell user about his empty gear slot, and updated stats
        sendInventoryMessage(user, slotNum, GEAR);
        user.updateStats();

    } else { // Alert object's watchers
        for (auto username : pEnt->watchers())
            pEnt->alertWatcherOnInventoryChange(*_usersByName[username], slotNum);
    }
}

void Server::handle_CL_LEAVE_CITY(User &user) {
    auto city = _cities.getPlayerCity(user.name());
    if (city.empty()) {
        sendMessage(user.socket(), SV_NOT_IN_CITY);
        return;
    }
    if (_kings.isPlayerAKing(user.name())) {
        sendMessage(user.socket(), SV_KING_CANNOT_LEAVE_CITY);
        return;
    }
    _cities.removeUserFromCity(user, city);
}

void Server::handle_CL_CEDE(User &user, size_t serial) {
    if (serial == INVENTORY || serial == GEAR) {
        sendMessage(user.socket(), SV_DOESNT_EXIST);
        return;
    }
    auto *obj = _entities.find<Object>(serial);

    if (!obj->permissions().isOwnedByPlayer(user.name())) {
        sendMessage(user.socket(), SV_NO_PERMISSION);
        return;
    }

    const City::Name &city = _cities.getPlayerCity(user.name());
    if (city.empty()) {
        sendMessage(user.socket(), SV_NOT_IN_CITY);
        return;
    }

    if (obj->objType().isPlayerUnique()) {
        sendMessage(user.socket(), SV_CANNOT_CEDE);
        return;
    }

    obj->permissions().setCityOwner(city);
    const Permissions::Owner &owner = obj->permissions().owner();
    for (const User *u : findUsersInArea(obj->location()))
        sendMessage(u->socket(), SV_OWNER, makeArgs(serial, owner.typeString(), owner.name));
}

void Server::handle_CL_PERFORM_OBJECT_ACTION(User & user, size_t serial, const std::string &textArg) {
    if (serial == INVENTORY || serial == GEAR) {
        sendMessage(user.socket(), SV_DOESNT_EXIST);
        return;
    }
    auto *obj = _entities.find<Object>(serial);

    if (!obj->permissions().isOwnedByPlayer(user.name())) {
        sendMessage(user.socket(), SV_NO_PERMISSION);
        return;
    }

    const auto &objType = obj->objType();
    if (!objType.hasAction()) {
        sendMessage(user.socket(), SV_NO_ACTION);
        return;
    }

    objType.action().function(*obj, user, textArg);
}

void Server::handle_CL_RECRUIT(User &user, const std::string & username) {
    const auto &cityName = _cities.getPlayerCity(user.name());
    if (cityName.empty()) {
        sendMessage(user.socket(), SV_NOT_IN_CITY);
        return;
    }
    if (!_cities.getPlayerCity(username).empty()) {
        sendMessage(user.socket(), SV_ALREADY_IN_CITY);
        return;
    }
    const auto *pTargetUser = getUserByName(username);
    if (pTargetUser == nullptr) {
        sendMessage(user.socket(), SV_INVALID_USER);
        return;
    }

    _cities.addPlayerToCity(*pTargetUser, cityName);
}


void Server::broadcast(MessageCode msgCode, const std::string &args){
    for (const User &user : _users){
        sendMessage(user.socket(), msgCode, args);
    }
}

void Server::broadcastToArea(const Point & location, MessageCode msgCode, const std::string & args) {
    for (const User *user : this->findUsersInArea(location)) {
        sendMessage(user->socket(), msgCode, args);
    }
}

void Server::sendMessage(const Socket &dstSocket, MessageCode msgCode,
                         const std::string &args) const{
    // Compile message
    std::ostringstream oss;
    oss << MSG_START << msgCode;
    if (args != "")
        oss << MSG_DELIM << args;
    oss << MSG_END;

    // Send message
    _socket.sendMessage(oss.str(), dstSocket);
}

void Server::sendInventoryMessageInner(const User &user, size_t serial, size_t slot,
                                      const ServerItem::vect_t &itemVect) const{
    if (slot >= itemVect.size()){
        sendMessage(user.socket(), SV_INVALID_SLOT);
        return;
    }
    const auto &containerSlot = itemVect[slot];
    std::string itemID = containerSlot.first ? containerSlot.first->id() : "none";
    sendMessage(user.socket(), SV_INVENTORY, makeArgs(serial, slot, itemID, containerSlot.second));
}

void Server::sendInventoryMessage(const User &user, size_t slot, const Object &obj) const{
    if (! obj.hasContainer()){
        assert(false);
        return;
    }
    sendInventoryMessageInner(user, obj.serial(), slot, obj.container().raw());
}

// Special serials only
void Server::sendInventoryMessage(const User &user, size_t slot, size_t serial) const{
    const ServerItem::vect_t *container = nullptr;
    switch (serial){
        case INVENTORY: container = &user.inventory();  break;
        case GEAR:      container = &user.gear();       break;
        default:
            assert(false);
    }
    sendInventoryMessageInner(user, serial, slot, *container);
}

void Server::sendMerchantSlotMessage(const User &user, const Object &obj, size_t slot) const{
    assert(slot < obj.merchantSlots().size());
    const MerchantSlot &mSlot = obj.merchantSlot(slot);
    if (mSlot)
        sendMessage(user.socket(), SV_MERCHANT_SLOT,
                    makeArgs(obj.serial(), slot,
                             mSlot.wareItem->id(), mSlot.wareQty,
                             mSlot.priceItem->id(), mSlot.priceQty));
    else
        sendMessage(user.socket(), SV_MERCHANT_SLOT, makeArgs(obj.serial(), slot, "", 0, "", 0));
}

void Server::sendConstructionMaterialsMessage(const User &user, const Object &obj) const{
    size_t n = obj.remainingMaterials().numTypes();
    std::string args = makeArgs(obj.serial(), n);
    for (const auto &pair : obj.remainingMaterials()){
        args = makeArgs(args, pair.first->id(), pair.second);
    }
    sendMessage(user.socket(), SV_CONSTRUCTION_MATERIALS, args);
}

void Server::sendNewBuildsMessage(const User &user, const std::set<std::string> &ids) const{
    if (!ids.empty()){ // New constructions unlocked!
        std::string args = makeArgs(ids.size());
        for (const std::string &id : ids)
            args = makeArgs(args, id);
        sendMessage(user.socket(), SV_NEW_CONSTRUCTIONS, args);
    }
}

void Server::sendNewRecipesMessage(const User &user, const std::set<std::string> &ids) const{
    if (!ids.empty()){ // New recipes unlocked!
        std::string args = makeArgs(ids.size());
        for (const std::string &id : ids)
            args = makeArgs(args, id);
        sendMessage(user.socket(), SV_NEW_RECIPES, args);
    }
}

void Server::alertUserToWar(const std::string &username, const Wars::Belligerent &otherBelligerent) const{
    auto it = _usersByName.find(username);
    if (it == _usersByName.end()) // user1 is offline
        return;

    const MessageCode code = otherBelligerent.type == Wars::Belligerent::CITY ?
            SV_AT_WAR_WITH_CITY : SV_AT_WAR_WITH_PLAYER;
    sendMessage(it->second->socket(), code, otherBelligerent.name);
}
