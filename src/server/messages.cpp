// (C) 2016 Tim Gurto

#include <cassert>
#include "Server.h"
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
            user->cancelAction();
            user->updateLocation(Point(x, y));
            broadcast(SV_LOCATION, user->makeLocationCommand());
            break;
        }

        case CL_CRAFT:
        {
            std::string id;
            iss.get(buffer, BUFFER_SIZE, MSG_END);
            id = std::string(buffer);
            iss >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();
            const std::set<Recipe>::const_iterator it = _recipes.find(id);
            if (it == _recipes.end()) {
                sendMessage(client, SV_INVALID_ITEM);
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
            const std::pair<const Item *, size_t> &invSlot = user->inventory(slot);
            if (invSlot.first == nullptr) {
                sendMessage(client, SV_EMPTY_SLOT);
                break;
            }
            const Item &item = *invSlot.first;
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
            std::set<Object>::iterator it = _objects.find(serial);
            if (!isValidObject(client, *user, it)) {
                sendMessage(client, SV_DOESNT_EXIST);
                break;
            }
            Object &obj = const_cast<Object &>(*it);
            assert (obj.type());
            // Check that the user meets the requirements
            if (!obj.userHasAccess(user->name())){
                sendMessage(client, SV_NO_PERMISSION);
                break;
            }
            const std::string &gatherReq = obj.type()->gatherReq();
            if (gatherReq != "none" && !user->hasTool(gatherReq)) {
                sendMessage(client, SV_ITEM_NEEDED, gatherReq);
                break;
            }
            // Check that it has no inventory
            if (!obj.container().empty()){
                sendMessage(client, SV_NOT_EMPTY);
                break;
            }
            user->beginGathering(&obj);
            sendMessage(client, SV_ACTION_STARTED, makeArgs(obj.type()->gatherTime()));
            break;
        }

        case CL_DECONSTRUCT:
        {
            int serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            user->cancelAction();
            std::set<Object>::iterator it = _objects.find(serial);
            if (!isValidObject(client, *user, it)) {
                sendMessage(client, SV_DOESNT_EXIST);
                break;
            }
            Object &obj = const_cast<Object &>(*it);
            assert (obj.type());
            if (!obj.userHasAccess(user->name())){
                sendMessage(client, SV_NO_PERMISSION);
                break;
            }
            // Check that the object can be deconstructed
            if (obj.type()->deconstructsItem() == nullptr){
                sendMessage(client, SV_CANNOT_DECONSTRUCT);
                break;
            }
            // Check that it has no inventory
            if (!obj.container().empty()){
                sendMessage(client, SV_NOT_EMPTY);
                break;
            }

            user->beginDeconstructing(obj);
            sendMessage(client, SV_ACTION_STARTED, makeArgs(obj.type()->deconstructionTime()));
            break;
        }

        case CL_DROP:
        {
            size_t serial, slot;
            iss >> serial >> del >> slot >> del;
            if (del != MSG_END)
                return;
            Item::vect_t *container;
            Object *pObj = nullptr;
            if (serial == 0)
                container = &user->inventory();
            else {
                auto it = _objects.find(serial);
                if (!isValidObject(client, *user, it))
                    break;
                pObj = &const_cast<Object&>(*it);
                container = &pObj->container();
            }

            if (slot >= container->size()) {
                sendMessage(client, SV_INVALID_SLOT);
                break;
            }
            auto &containerSlot = (*container)[slot];
            if (containerSlot.second != 0) {
                containerSlot.first = nullptr;
                containerSlot.second = 0;
                if (serial == 0)
                    sendInventoryMessage(*user, slot);
                else{
                    for (auto username : pObj->watchers())
                         if (pObj->userHasAccess(username))
                            sendInventoryMessage(*_usersByName[username], slot, pObj);
                }
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
            Item::vect_t
                *containerFrom,
                *containerTo;
            Object
                *pObj1 = nullptr,
                *pObj2 = nullptr;
            if (obj1 == 0)
                containerFrom = &user->inventory();
            else {
                auto it = _objects.find(obj1);
                if (!isValidObject(client, *user, it))
                    break;
                pObj1 = &const_cast<Object&>(*it);
                containerFrom = &pObj1->container();
            }
            if (obj2 == 0)
                containerTo = &user->inventory();
            else {
                auto it = _objects.find(obj2);
                if (!isValidObject(client, *user, it))
                    break;
                pObj2 = &const_cast<Object&>(*it);
                containerTo = &pObj2->container();
            }

            if (slot1 >= containerFrom->size() || slot2 >= containerTo->size()) {
                sendMessage(client, SV_INVALID_SLOT);
                break;
            }

            // Perform the swap
            auto
                &slotFrom = (*containerFrom)[slot1],
                &slotTo = (*containerTo)[slot2];
            auto temp = slotTo;
            slotTo = slotFrom;
            slotFrom = temp;

            if (obj1 == 0)
                sendInventoryMessage(*user, slot1);
            if (obj2 == 0)
                sendInventoryMessage(*user, slot2);
            
            // Alert watchers
            if (obj1 != 0) {
                for (auto username : pObj1->watchers())
                    if (pObj1->userHasAccess(username))
                        sendInventoryMessage(*_usersByName[username], slot1, pObj1);
            }
            if (obj2 != 0) {
                for (auto username : pObj2->watchers())
                    if (pObj2->userHasAccess(username))
                        sendInventoryMessage(*_usersByName[username], slot2, pObj2);
            }
            break;
        }

        case CL_TRADE:
        {
            size_t serial, slot;
            iss >> serial >> del >> slot >> del;
            if (del != MSG_END)
                return;

            // Check that merchant slot is valid
            auto it = _objects.find(serial);
            if (!isValidObject(client, *user, it))
                break;
            Object &obj = const_cast<Object &>(*it);
            size_t slots = obj.type()->merchantSlots();
            if (slots == 0){
                sendMessage(client, SV_NOT_MERCHANT);
                break;
            } else if (slot >= slots){
                sendMessage(client, SV_INVALID_MERCHANT_SLOT);
                break;
            }
            const MerchantSlot &mSlot = obj.merchantSlot(slot);
            if (!mSlot){
                sendMessage(client, SV_INVALID_MERCHANT_SLOT);
                break;
            }

            // Check that object has items in stock
            if (mSlot.ware() > obj.container()){
                sendMessage(client, SV_NO_WARE);
                break;
            }

            // Check that user has price
            if (mSlot.price() > user->inventory()){
                sendMessage(client, SV_NO_PRICE);
                break;
            }

            // Check that user has inventory space
            if (!vectHasSpace(user->inventory(), mSlot.wareItem, mSlot.wareQty)){
                sendMessage(client, SV_INVENTORY_FULL);
                break;
            }

            // Check that object has inventory space
            if (!vectHasSpace(obj.container(), mSlot.wareItem, mSlot.wareQty)){
                sendMessage(client, SV_MERCHANT_INVENTORY_FULL);
                break;
            }

            // Take price from user
            user->removeItems(mSlot.price());

            // Take ware from object
            obj.removeItems(mSlot.ware());

            // Give price to object
            obj.giveItem(mSlot.priceItem, mSlot.priceQty);

            // Give ware to user
            user->giveItem(mSlot.wareItem, mSlot.wareQty);

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
            auto objIt = _objects.find(serial);
            if (!isValidObject(client, *user, objIt))
                break;
            Object &obj = const_cast<Object &>(*objIt);
            if (!obj.userHasAccess(user->name())){
                sendMessage(client, SV_NO_PERMISSION);
                break;
            }
            size_t slots = obj.type()->merchantSlots();
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
            MerchantSlot &mSlot = obj.merchantSlot(slot);
            mSlot = MerchantSlot(&*wareIt, wareQty, &*priceIt, priceQty);

            // Alert watchers
            for (auto username : obj.watchers())
                sendMerchantSlotMessage(*_usersByName[username], obj, slot);
            break;
        }

        case CL_CLEAR_MERCHANT_SLOT:
        {
            size_t serial, slot;
            iss >> serial >> del >> slot >> del;
            if (del != MSG_END)
                return;
            auto objIt = _objects.find(serial);
            if (!isValidObject(client, *user, objIt))
                break;
            Object &obj = const_cast<Object &>(*objIt);
            if (!obj.userHasAccess(user->name())){
                sendMessage(client, SV_NO_PERMISSION);
                break;
            }
            size_t slots = obj.type()->merchantSlots();
            if (slots == 0){
                sendMessage(client, SV_NOT_MERCHANT);
                break;
            }
            if (slot >= slots){
                sendMessage(client, SV_INVALID_MERCHANT_SLOT);
                break;
            }
            obj.merchantSlot(slot) = MerchantSlot();

            // Alert watchers
            for (auto username : obj.watchers())
                sendMerchantSlotMessage(*_usersByName[username], obj, slot);
            break;
        }

        case CL_START_WATCHING:
        {
            size_t serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            auto it = _objects.find(serial);
            if (!isValidObject(client, *user, it))
                break;
            Object &obj = const_cast<Object &>(*it);

            // Describe merchant slots, if any
            size_t mSlots = obj.merchantSlots().size();
            for (size_t i = 0; i != mSlots; ++i)
                sendMerchantSlotMessage(*user, obj, i);

            // Describe inventory, if user has permission
            if (obj.userHasAccess(user->name())){
                size_t slots = it->container().size();
                for (size_t i = 0; i != slots; ++i)
                    sendInventoryMessage(*user, i, &obj);
            }

            // Add as watcher
            obj.addWatcher(user->name());

            break;
        }

        case CL_STOP_WATCHING:
        {
            size_t serial;
            iss >> serial >> del;
            if (del != MSG_END)
                return;
            auto it = _objects.find(serial);
            if (it == _objects.end()) {
                sendMessage(client, SV_DOESNT_EXIST);
                break;
            }
            Object &obj = const_cast<Object &>(*it);

            obj.removeWatcher(user->name());

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

        default:
            _debug << Color::RED << "Unhandled message: " << msg << Log::endl;
        }
    }
}

void Server::broadcast(MessageCode msgCode, const std::string &args){
    for (const User &user : _users){
        sendMessage(user.socket(), msgCode, args);
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

void Server::sendInventoryMessage(const User &user, size_t slot, const Object *obj) const{
    const Item::vect_t &container = (obj == nullptr) ? user.inventory() : obj->container();
    if (slot >= container.size()) {
        sendMessage(user.socket(), SV_INVALID_SLOT);
        return;
    }
    size_t serial = obj == nullptr ? 0 : obj->serial();
    auto containerSlot = container[slot];
    std::string itemID = containerSlot.first ? containerSlot.first->id() : "none";
    sendMessage(user.socket(), SV_INVENTORY, makeArgs(serial, slot, itemID, containerSlot.second));
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
