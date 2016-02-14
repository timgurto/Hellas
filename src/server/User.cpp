// (C) 2015 Tim Gurto

#include <cassert>
#include <sstream>

#include "Item.h"
#include "ItemSet.h"
#include "ObjectType.h"
#include "Server.h"
#include "User.h"
#include "../Socket.h"
#include "../messageCodes.h"
#include "../util.h"

const size_t User::INVENTORY_SIZE = 10;

const ObjectType User::OBJECT_TYPE(Rect(-5, -2, 10, 4));

User::User(const std::string &name, const Point &loc, const Socket &socket):
_name(name),
_socket(socket),
_location(loc),
_actionTarget(0),
_actionCrafting(0),
_actionConstructing(0),
_constructingSlot(INVENTORY_SIZE),
_constructingLocation(0, 0),
_actionTime(0),
_inventory(INVENTORY_SIZE),
_lastLocUpdate(SDL_GetTicks()),
_lastContact(SDL_GetTicks()){
    for (size_t i = 0; i != INVENTORY_SIZE; ++i)
        _inventory[i] = std::make_pair<const Item *, size_t>(0, 0);
}

User::User(const Socket &rhs):
_socket(rhs){}

const std::pair<const Item *, size_t> &User::inventory(size_t index) const{
    return _inventory[index];
}

std::pair<const Item *, size_t> &User::inventory(size_t index){
    return _inventory[index];
}

std::string User::makeLocationCommand() const{
    return makeArgs(_name, _location.x, _location.y);
}

void User::updateLocation(const Point &dest){
    const Uint32 newTime = SDL_GetTicks();
    Uint32 timeElapsed = newTime - _lastLocUpdate;
    _lastLocUpdate = newTime;

    // Max legal distance: straight line
    const double maxLegalDistance = min(Server::MAX_TIME_BETWEEN_LOCATION_UPDATES,
                                        timeElapsed + 100)
                                    / 1000.0 * Server::MOVEMENT_SPEED;
    Point interpolated = interpolate(_location, dest, maxLegalDistance);

    Point newDest = interpolated;
    Server &server = *Server::_instance;
    if (!server.isLocationValid(newDest, OBJECT_TYPE, 0, this)) {
        newDest = _location;
        static const double ACCURACY = 0.5;
        Point testPoint = _location;
        const bool xDeltaPositive = _location.x < interpolated.x;
        do {
            newDest.x = testPoint.x;
            testPoint.x = xDeltaPositive ? (testPoint.x) + ACCURACY : (testPoint.x - ACCURACY);
        } while ((xDeltaPositive ? (testPoint.x <= interpolated.x) :
                                   (testPoint.x >= interpolated.x)) &&
                 server.isLocationValid(testPoint, OBJECT_TYPE, 0, this));
        const bool yDeltaPositive = _location.y < interpolated.y;
        testPoint.x = newDest.x; // Keep it valid for y testing.
        do {
            newDest.y = testPoint.y;
            testPoint.y = yDeltaPositive ? (testPoint.y + ACCURACY) : (testPoint.y - ACCURACY);
        } while ((yDeltaPositive ? (testPoint.y <= interpolated.y) :
                                   (testPoint.y >= interpolated.y)) &&
                 server.isLocationValid(testPoint, OBJECT_TYPE, 0, this));
    }

    _location = newDest;
}

void User::contact(){
    _lastContact = SDL_GetTicks();
}

bool User::alive() const{
    return SDL_GetTicks() - _lastContact <= Server::CLIENT_TIMEOUT;
}

bool User::hasSpace(const Item *item, size_t quantity) const{
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (!_inventory[i].first) {
            if (quantity <= item->stackSize())
                return true;
            quantity -= item->stackSize();
        } else if (_inventory[i].first == item) {
            size_t spaceAvailable = item->stackSize() - _inventory[i].second;
            if (quantity <= spaceAvailable)
                return true;
            quantity -= spaceAvailable;
        } else
            continue;
    }
    return false;
}

size_t User::giveItem(const Item *item, size_t quantity){
    // First pass: partial stacks
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first != item)
            continue;
        size_t spaceAvailable = item->stackSize() - _inventory[i].second;
        if (spaceAvailable > 0) {
            size_t qtyInThisSlot = min(spaceAvailable, quantity);
            _inventory[i].second += qtyInThisSlot;
            Server::instance().sendInventoryMessage(*this, 0, i);
            quantity -= qtyInThisSlot;
        }
        if (quantity == 0)
            return 0;
    }

    // Second pass: empty slots
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first)
            continue;
        size_t qtyInThisSlot = min(item->stackSize(), quantity);
        _inventory[i].first = item;
        _inventory[i].second = qtyInThisSlot;
        Server::instance().sendInventoryMessage(*this, 0, i);
        quantity -= qtyInThisSlot;
        if (quantity == 0)
            return 0;
    }
    return quantity;
}

void User::cancelAction() {
    if (_actionTarget || _actionCrafting)
        Server::instance().sendMessage(_socket, SV_ACTION_INTERRUPTED);
    _actionTarget = 0;
    _actionCrafting = 0;
    _actionTime = 0;
}

void User::actionTarget(const Object *obj){
    _actionTarget = obj;
    assert(obj->type());
    _actionTime = obj->type()->actionTime();
}

void User::actionCraft(const Recipe &recipe){
    _actionCrafting = &recipe;
    _actionTime = recipe.time();
}

void User::actionConstruct(const ObjectType &obj, const Point &location, size_t slot){
    _actionConstructing = &obj;
    _actionTime = obj.constructionTime();
    _constructingSlot = slot;
    _constructingLocation = location;
}

bool User::hasItems(const ItemSet &items) const{
    ItemSet remaining = items;
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        const std::pair<const Item *, size_t> &invSlot = _inventory[i];
        remaining.remove(invSlot.first, invSlot.second);
        if (remaining.isEmpty())
            return true;
    }
    return false;
}

bool User::hasTool(const std::string &className) const{
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        const Item *item = _inventory[i].first;
        if (item && item->isClass(className))
            return true;
    }

    // Check nearby objects
    auto superChunk = Server::_instance->getCollisionSuperChunk(_location);
    for (CollisionChunk *chunk : superChunk)
        for (const auto &ret : chunk->objects()) {
            const Object *pObj = ret.second;
            if (pObj->type()->isClass(className) &&
                distance(pObj->collisionRect(), collisionRect()) < Server::ACTION_DISTANCE)
                return true;
        }

    return false;
}

bool User::hasTools(const std::set<std::string> &classes) const{
    for (const std::string &className : classes)
        if (!hasTool(className))
            return false;
    return true;
}

void User::removeItems(const ItemSet &items) {
    std::set<size_t> invSlotsChanged;
    ItemSet remaining = items;
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        std::pair<const Item *, size_t> &invSlot = _inventory[i];
        if (remaining.contains(invSlot.first)) {
            size_t itemsToRemove = min(invSlot.second, remaining[invSlot.first]);
            remaining.remove(invSlot.first, itemsToRemove);
            _inventory[i].second -= itemsToRemove;
            if (_inventory[i].second == 0)
                _inventory[i].first = 0;
            invSlotsChanged.insert(i);
            if (remaining.isEmpty())
                break;
        }
    }
    for (size_t slotNum : invSlotsChanged) {
        const std::pair<const Item *, size_t> &slot = _inventory[slotNum];
        std::string id = slot.first ? slot.first->id() : "none";
        Server::instance().sendInventoryMessage(*this, 0, slotNum);
    }
}

void User::update(Uint32 timeElapsed){
    if (_actionTime == 0)
        return;
    Server &server = *Server::_instance;
    if (_actionTime > timeElapsed)
        _actionTime -= timeElapsed;
    else {
        if (_actionTarget) {
            server.gatherObject(_actionTarget->serial(), *this);
            _actionTarget = 0;
        } else if (_actionCrafting) {
            if (!hasSpace(_actionCrafting->product())) {
                Server::instance().sendMessage(_socket, SV_INVENTORY_FULL);
                return;
            }
            // Give user his newly crafted item
            giveItem(_actionCrafting->product(), 1);
            // Remove materials from user's inventory
            removeItems(_actionCrafting->materials());
            _actionCrafting = 0;
        } else if (_actionConstructing) {
            // Create object
            server.addObject(_actionConstructing, _constructingLocation, this);

            // Remove item from user's inventory
            std::pair<const Item *, size_t> &slot = _inventory[_constructingSlot];
            assert(slot.first->constructsObject() == _actionConstructing);
            --slot.second;
            if (slot.second == 0)
                slot.first = 0;
            server.sendInventoryMessage(*this, 0, _constructingSlot);
        }
        _actionTime = 0;
        server.sendMessage(_socket, SV_ACTION_FINISHED);
    }
}

const Rect User::collisionRect() const{
    return OBJECT_TYPE.collisionRect() + _location;
}
