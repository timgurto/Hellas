#include <cassert>
#include <sstream>

#include "ServerItem.h"
#include "ItemSet.h"
#include "ObjectType.h"
#include "Server.h"
#include "User.h"
#include "../Socket.h"
#include "../messageCodes.h"
#include "../util.h"

const size_t User::INVENTORY_SIZE = 10;
const size_t User::GEAR_SLOTS = 4;

const ObjectType User::OBJECT_TYPE(Rect(-5, -2, 10, 4));

const health_t User::MAX_HEALTH = 100;
const health_t User::ATTACK_DAMAGE = 8;
const ms_t User::ATTACK_TIME = 1000;

User::User(const std::string &name, const Point &loc, const Socket &socket):
Combatant(MAX_HEALTH),

_name(name),
_socket(socket),
_location(loc),

_action(NO_ACTION),
_actionTime(0),
_attackTime(0),
_actionObject(nullptr),
_actionRecipe(nullptr),
_actionObjectType(nullptr),
_actionSlot(INVENTORY_SIZE),
_actionLocation(0, 0),
_actionNPC(nullptr),

_inventory(INVENTORY_SIZE),
_gear(GEAR_SLOTS),
_lastLocUpdate(SDL_GetTicks()),
_lastContact(SDL_GetTicks()){
    for (size_t i = 0; i != INVENTORY_SIZE; ++i)
        _inventory[i] = std::make_pair<const ServerItem *, size_t>(0, 0);
}

User::User(const Socket &rhs):
_socket(rhs){}

User::User(const Point &loc):
_location(loc){}

bool User::compareXThenSerial::operator()( const User *a, const User *b){
    if (a->_location.x != b->_location.x)
        return a->_location.x < b->_location.x;
    return a->_socket < b->_socket; // Just need something unique.
}

bool User::compareYThenSerial::operator()( const User *a, const User *b){
    if (a->_location.y != b->_location.y)
        return a->_location.y < b->_location.y;
    return a->_socket < b->_socket; // Just need something unique.
}

std::string User::makeLocationCommand() const{
    return makeArgs(_name, _location.x, _location.y);
}

void User::updateLocation(const Point &dest){
    const ms_t newTime = SDL_GetTicks();
    ms_t timeElapsed = newTime - _lastLocUpdate;
    _lastLocUpdate = newTime;

    // Max legal distance: straight line
    const double maxLegalDistance = min(Server::MAX_TIME_BETWEEN_LOCATION_UPDATES,
                                        timeElapsed + 100)
                                    / 1000.0 * Server::MOVEMENT_SPEED;
    Point interpolated = interpolate(_location, dest, maxLegalDistance);

    Point newDest = interpolated;
    Server &server = *Server::_instance;

    int
        displacementX = toInt(newDest.x - _location.x),
        displacementY = toInt(newDest.y - _location.y);
    Rect journeyRect = collisionRect();
    if (displacementX < 0) {
        journeyRect.x += displacementX;
        journeyRect.w -= displacementX;
    } else {
        journeyRect.w += displacementX;
    }
    if (displacementY < 0) {
        journeyRect.y += displacementY;
        journeyRect.h -= displacementY;
    } else {
        journeyRect.h += displacementY;
    }
    if (!server.isLocationValid(journeyRect, nullptr, this)) {
        newDest = _location;
        static const double ACCURACY = 0.5;
        Point testPoint = _location;
        const bool xDeltaPositive = _location.x < interpolated.x;
        do {
            newDest.x = testPoint.x;
            testPoint.x = xDeltaPositive ? (testPoint.x) + ACCURACY : (testPoint.x - ACCURACY);
        } while ((xDeltaPositive ? (testPoint.x <= interpolated.x) :
                                   (testPoint.x >= interpolated.x)) &&
                 server.isLocationValid(testPoint, OBJECT_TYPE, nullptr, this));
        const bool yDeltaPositive = _location.y < interpolated.y;
        testPoint.x = newDest.x; // Keep it valid for y testing.
        do {
            newDest.y = testPoint.y;
            testPoint.y = yDeltaPositive ? (testPoint.y + ACCURACY) : (testPoint.y - ACCURACY);
        } while ((yDeltaPositive ? (testPoint.y <= interpolated.y) :
                                   (testPoint.y >= interpolated.y)) &&
                 server.isLocationValid(testPoint, OBJECT_TYPE, nullptr, this));
    }

    // At this point, the user's new location has been finalized.
    server.sendMessage(socket(), SV_LOCATION, makeArgs(_name, newDest.x, newDest.y));

    // Tell user about any additional objects he can now see
    std::list<const Object *> nearbyObjects;
    double left, right, top, bottom;
    double forgetLeft, forgetRight, forgetTop, forgetBottom; // Areas newly invisible
    if (newDest.x > _location.x){ // Moved right
        left = _location.x + Server::CULL_DISTANCE;
        right = newDest.x + Server::CULL_DISTANCE;
        forgetLeft = _location.x - Server::CULL_DISTANCE;
        forgetRight = newDest.x - Server::CULL_DISTANCE;
    } else { // Moved left
        left = newDest.x - Server::CULL_DISTANCE;
        right = _location.x - Server::CULL_DISTANCE;
        forgetLeft = newDest.x + Server::CULL_DISTANCE;
        forgetRight = _location.x + Server::CULL_DISTANCE;
    }
    if (newDest.y > _location.y){ // Moved down
        top = _location.y + Server::CULL_DISTANCE;
        bottom = newDest.y + Server::CULL_DISTANCE;
        forgetTop = _location.y - Server::CULL_DISTANCE;
        forgetBottom = newDest.y - Server::CULL_DISTANCE;
    } else { // Moved up
        top = newDest.y - Server::CULL_DISTANCE;
        bottom = _location.y - Server::CULL_DISTANCE;
        forgetTop = newDest.y + Server::CULL_DISTANCE;
        forgetBottom = _location.y + Server::CULL_DISTANCE;
    }
    auto loX = server._objectsByX.lower_bound(&Object(Point(left, 0)));
    auto hiX = server._objectsByX.upper_bound(&Object(Point(right, 0)));
    auto loY = server._objectsByY.lower_bound(&Object(Point(0, top)));
    auto hiY = server._objectsByY.upper_bound(&Object(Point(0, bottom)));
    for (auto it = loX; it != hiX; ++it){
        if (abs((*it)->location().y - newDest.y) <= Server::CULL_DISTANCE)
            nearbyObjects.push_back(*it);
    }
    for (auto it = loY; it != hiY; ++it){
        double objX = (*it)->location().x;
        if (newDest.x > _location.x){ // Don't count objects twice
            if (objX > left)
                continue;
        } else {
            if (objX < right)
                continue;
        }
        if (abs(objX - newDest.x) <= Server::CULL_DISTANCE)
            nearbyObjects.push_back(*it);
    }
    for (const Object *objP : nearbyObjects){
        server.sendObjectInfo(*this, *objP);
    }

    // Tell user about any additional users he can now see
    auto loUserX = server._usersByX.lower_bound(&User(Point(left, 0)));
    auto hiUserX = server._usersByX.upper_bound(&User(Point(right, 0)));
    auto loUserY = server._usersByY.lower_bound(&User(Point(0, top)));
    auto hiUserY = server._usersByY.upper_bound(&User(Point(0, bottom)));
    std::list<const User *> nearbyUsers;
    for (auto it = loUserX; it != hiUserX; ++it){
        double userY = (*it)->location().y;
        if (userY - newDest.y <= Server::CULL_DISTANCE)
            nearbyUsers.push_back(*it);
    }
    for (auto it = loUserY; it != hiUserY; ++it){
        double userX = (*it)->location().x;
        if (newDest.x > _location.x){ // Don't count objects twice
            if (userX > left)
                continue;
        } else {
            if (userX < right)
                continue;
        }
        if (abs(userX - newDest.x) <= Server::CULL_DISTANCE)
            nearbyUsers.push_back(*it);
    }
    for (const User *userP : nearbyUsers)
        server.sendMessage(socket(), SV_LOCATION, userP->makeLocationCommand());

    // Tell any users he has moved away from to forget about him.
   loUserX = server._usersByX.lower_bound(&User(Point(forgetLeft, 0)));
   hiUserX = server._usersByX.upper_bound(&User(Point(forgetRight, 0)));
   loUserY = server._usersByY.lower_bound(&User(Point(0, forgetTop)));
   hiUserY = server._usersByY.upper_bound(&User(Point(0, forgetBottom)));
   std::list<const User *> usersToForget;
    for (auto it = loUserX; it != hiUserX; ++it){
        double userY = (*it)->location().y;
        if (userY - _location.y <= Server::CULL_DISTANCE)
            usersToForget.push_back(*it);
    }
    for (auto it = loUserY; it != hiUserY; ++it){
        double userX = (*it)->location().x;
        if (newDest.x > _location.x){ // Don't count objects twice.
            if (userX < forgetLeft)
                continue;
        } else {
            if (userX > forgetRight)
                continue;
        }
        if (abs(userX - _location.x) <= Server::CULL_DISTANCE)
            usersToForget.push_back(*it);
    }
    for (const User *userP : usersToForget)
        server.sendMessage(userP->socket(), SV_USER_OUT_OF_RANGE, name());

    Point oldLoc = _location;

    // Remove from location-indexed trees
    if (newDest.x != oldLoc.x)
        server._usersByX.erase(this);
    if (newDest.y != oldLoc.y)
        server._usersByY.erase(this);

    _location = newDest;

    // Re-insert into location-indexed trees
    if (newDest.x != oldLoc.x)
        server._usersByX.insert(this);
    if (newDest.y != oldLoc.y)
        server._usersByY.insert(this);
}

void User::contact(){
    _lastContact = SDL_GetTicks();
}

bool User::alive() const{
    return SDL_GetTicks() - _lastContact <= Server::CLIENT_TIMEOUT;
}

size_t User::giveItem(const ServerItem *item, size_t quantity){
    // First pass: partial stacks
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first != item)
            continue;
        size_t spaceAvailable = item->stackSize() - _inventory[i].second;
        if (spaceAvailable > 0) {
            size_t qtyInThisSlot = min(spaceAvailable, quantity);
            _inventory[i].second += qtyInThisSlot;
            Server::instance().sendInventoryMessage(*this, i, Server::INVENTORY);
            quantity -= qtyInThisSlot;
        }
        if (quantity == 0)
            return 0;
    }

    // Second pass: empty slots
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first != nullptr)
            continue;
        size_t qtyInThisSlot = min(item->stackSize(), quantity);
        _inventory[i].first = item;
        _inventory[i].second = qtyInThisSlot;
        Server::instance().sendInventoryMessage(*this, i, Server::INVENTORY);
        quantity -= qtyInThisSlot;
        if (quantity == 0)
            return 0;
    }
    return quantity;
}

void User::cancelAction() {
    if (_action == NO_ACTION)
        return;

    switch(_action){
    case GATHER:
        _actionObject->decrementGatheringUsers();
    }

    Server::instance().sendMessage(_socket, SV_ACTION_INTERRUPTED);
    _action = NO_ACTION;
}

void User::finishAction() {
    if (_action == NO_ACTION)
        return;

    _action = NO_ACTION;
}

void User::beginGathering(Object *obj){
    _action = GATHER;
    _actionObject = obj;
    _actionObject->incrementGatheringUsers();
    assert(obj->type());
    _actionTime = obj->type()->gatherTime();
}

void User::beginCrafting(const Recipe &recipe){
    _action = CRAFT;
    _actionRecipe = &recipe;
    _actionTime = recipe.time();
}

void User::beginConstructing(const ObjectType &obj, const Point &location, size_t slot){
    _action = CONSTRUCT;
    _actionObjectType = &obj;
    _actionTime = obj.constructionTime();
    _actionSlot = slot;
    _actionLocation = location;
}

void User::beginDeconstructing(Object &obj){
    _action = DECONSTRUCT;
    _actionObject = &obj;
    _actionTime = obj.type()->deconstructionTime();
}

void User::targetNPC(NPC *npc){
    _actionNPC = npc;
    if (_actionNPC == nullptr){
        cancelAction();
        return;
    }
    _action = ATTACK;
}

bool User::hasItems(const ItemSet &items) const{
    ItemSet remaining = items;
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        const std::pair<const ServerItem *, size_t> &invSlot = _inventory[i];
        remaining.remove(invSlot.first, invSlot.second);
        if (remaining.isEmpty())
            return true;
    }
    return false;
}

bool User::hasTool(const std::string &className) const{
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        const ServerItem *item = _inventory[i].first;
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
        std::pair<const ServerItem *, size_t> &invSlot = _inventory[i];
        if (remaining.contains(invSlot.first)) {
            size_t itemsToRemove = min(invSlot.second, remaining[invSlot.first]);
            remaining.remove(invSlot.first, itemsToRemove);
            _inventory[i].second -= itemsToRemove;
            if (_inventory[i].second == 0)
                _inventory[i].first = nullptr;
            invSlotsChanged.insert(i);
            if (remaining.isEmpty())
                break;
        }
    }
    for (size_t slotNum : invSlotsChanged) {
        const std::pair<const ServerItem *, size_t> &slot = _inventory[slotNum];
        std::string id = slot.first ? slot.first->id() : "none";
        Server::instance().sendInventoryMessage(*this, slotNum, Server::INVENTORY);
    }
}

void User::update(ms_t timeElapsed){
    if (_action == NO_ACTION)
        return;

    if (_actionTime > timeElapsed)
        _actionTime -= timeElapsed;
    else
        _actionTime = 0;

    if (_attackTime > timeElapsed)
        _attackTime -= timeElapsed;
    else
        _attackTime = 0;

    Server &server = *Server::_instance;


    // Attack actions:

    if (_action == ATTACK){
        assert(_actionNPC->health() > 0);

        if (_attackTime > 0)
            return;

        // Check if within range
        if (distance(collisionRect(), _actionNPC->collisionRect()) <= Server::ACTION_DISTANCE){

            // Reduce target health (to minimum 0)
            health_t remaining = _actionNPC->reduceHealth(ATTACK_DAMAGE);
            for (const User *user: server.findUsersInArea(_actionNPC->location()))
                server.sendMessage(user->socket(), SV_NPC_HEALTH,
                                   makeArgs(_actionNPC->serial(), remaining));

            // Reset timer
            _attackTime = ATTACK_TIME;
        }
        return;
    }


    // Non-attack actions:

    if (_actionTime > 0) // Action hasn't finished yet.
        return;
    
    // Timer has finished; complete action
    switch(_action){
    case GATHER:
        server.gatherObject(_actionObject->serial(), *this);
        break;

    case CRAFT:
    {
        const ServerItem *product = toServerItem(_actionRecipe->product());
        if (!vectHasSpace(_inventory, product)) {
            server.sendMessage(_socket, SV_INVENTORY_FULL);
            cancelAction();
            return;
        }
        // Give user his newly crafted item
        giveItem(product, 1);
        // Remove materials from user's inventory
        removeItems(_actionRecipe->materials());
        break;
    }

    case CONSTRUCT:
    {
        // Create object
        server.addObject(_actionObjectType, _actionLocation, this);
        // Remove item from user's inventory
        std::pair<const ServerItem *, size_t> &slot = _inventory[_actionSlot];
        assert(slot.first->constructsObject() == _actionObjectType);
        --slot.second;
        if (slot.second == 0)
            slot.first = nullptr;
        server.sendInventoryMessage(*this, _actionSlot, Server::INVENTORY);
        break;
    }

    case DECONSTRUCT:
    {
        //Check for inventory space
        const ServerItem *item = _actionObject->type()->deconstructsItem();
        if (!vectHasSpace(_inventory, item)){
            server.sendMessage(_socket, SV_INVENTORY_FULL);
            cancelAction();
            return;
        }
        // Give user his item
        giveItem(item);
        // Remove object
        server.removeObject(*_actionObject);
        break;
    }

    default:
        assert(false);
    }
    
    if (_action != ATTACK){ // ATTACK is a repeating action.
        server.sendMessage(_socket, SV_ACTION_FINISHED);
        finishAction();
    }
}

const Rect User::collisionRect() const{
    return OBJECT_TYPE.collisionRect() + _location;
}
