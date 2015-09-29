// (C) 2015 Tim Gurto

#include <cassert>
#include <sstream>

#include "Client.h"
#include "Item.h"
#include "Server.h"
#include "Socket.h"
#include "User.h"
#include "messageCodes.h"
#include "util.h"

const size_t User::INVENTORY_SIZE = 5;

User::User(const std::string &name, const Point &loc, const Socket &socket):
_name(name),
_socket(socket),
_location(loc),
_actionTarget(0),
_actionCrafting(0),
_actionTime(0),
_inventory(INVENTORY_SIZE),
_lastLocUpdate(SDL_GetTicks()),
_lastContact(SDL_GetTicks()){
    for (size_t i = 0; i != INVENTORY_SIZE; ++i)
        _inventory[i] = std::make_pair<const Item *, size_t>(0, 0);
}

User::User(const Socket &rhs):
_socket(rhs){}

void User::location(std::istream &is){
    is >> _location.x >> _location.y;
}

const std::pair<const Item *, size_t> &User::inventory(size_t index) const{
    return _inventory[index];
}

std::pair<const Item *, size_t> &User::inventory(size_t index){
    return _inventory[index];
}

std::string User::makeLocationCommand() const{
    return makeArgs(_name, _location.x, _location.y);
}

void User::updateLocation(const Point &dest, const Server &server){
    const Uint32 newTime = SDL_GetTicks();
    Uint32 timeElapsed = newTime - _lastLocUpdate;
    _lastLocUpdate = newTime;

    // Max legal distance: straight line
    const double maxLegalDistance = min(Server::MAX_TIME_BETWEEN_LOCATION_UPDATES,
                                        timeElapsed + 100)
                                    / 1000.0 * Server::MOVEMENT_SPEED;
    _location = interpolate(_location, dest, maxLegalDistance);

    // Keep in-bounds
    const int
        xLimit = server.mapX() * Server::TILE_W - Server::TILE_W/2,
        yLimit = server.mapY() * Server::TILE_H;
    if (_location.x < 0)
        _location.x = 0;
    else if (_location.x > xLimit)
        _location.x = xLimit;
    if (_location.y < 0)
        _location.y = 0;
    else if (_location.y > yLimit)
        _location.y = yLimit;
}

void User::contact(){
    _lastContact = SDL_GetTicks();
}

bool User::alive() const{
    return SDL_GetTicks() - _lastContact <= Server::CLIENT_TIMEOUT;
}

size_t User::giveItem(const Item *item){
    size_t emptySlot = INVENTORY_SIZE;
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        const Item *invItem = _inventory[i].first;
        size_t invQty = _inventory[i].second;
        if (invItem == item && invQty < item->stackSize()){
            ++_inventory[i].second;
            return i;
        }
        if (!invItem && emptySlot == INVENTORY_SIZE)
            emptySlot = i;
    }
    if (emptySlot != INVENTORY_SIZE) {
        _inventory[emptySlot] = std::make_pair(item, 1);
    }
    return emptySlot;
}

void User::cancelAction(Server &server) {
    if (_actionTarget || _actionCrafting)
        server.sendMessage(_socket, SV_ACTION_INTERRUPTED);
    _actionTarget = 0;
    _actionCrafting = 0;
    _actionTime = 0;
}

void User::actionTarget(const Object *obj){
    _actionTarget = obj;
    assert(obj->type());
    _actionTime = obj->type()->actionTime();
}

void User::actionCraft(const Item &item){
    _actionCrafting = &item;
    _actionTime = item.craftTime();
}

bool User::hasMaterials(const Item &item) const{
    std::map<const Item *, size_t> remaining = item.materials();
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        const std::pair<const Item *, size_t> &invSlot = _inventory[i];
        std::map<const Item *, size_t>::iterator it = remaining.find(invSlot.first);
        if (it != remaining.end()) {
            // Subtract this slot's stack from remaining materials needed
            if (it->second <= invSlot.second)
                remaining.erase(it);
            else
                it->second -= invSlot.second;
        }
    }
    return remaining.empty();
}

bool User::hasItemClass(const std::string &className) const{
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        const Item *const item = _inventory[i].first;
        if (item->isClass(className))
            return true;
    }
    return false;
}

void User::removeMaterials(const Item &item, Server &server) {
    std::set<size_t> invSlotsChanged;
    std::map<const Item*, size_t> remaining = item.materials();
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        std::pair<const Item *, size_t> &invSlot = _inventory[i];
        const std::map<const Item *, size_t>::iterator it = remaining.find(invSlot.first);
        if (it != remaining.end()) {
            // Subtract this slot's stack from remaining materials needed
            if (it->second <= invSlot.second) {
                invSlot.second -= it->second;
                invSlotsChanged.insert(i);
                remaining.erase(it);
            } else {
                it->second -= invSlot.second;
            }
            if (invSlot.second == 0) {
                invSlot.first = 0;
                invSlotsChanged.insert(i);
            }
        }
    }
    for (size_t slotNum : invSlotsChanged) {
        const std::pair<const Item *, size_t> &slot = _inventory[slotNum];
        server.sendMessage(_socket, SV_INVENTORY, makeArgs(slotNum, slot.first->id(), slot.second));
    }
}

void User::update(Uint32 timeElapsed, Server &server){
    if (_actionTime == 0)
        return;
    if (_actionTime > timeElapsed)
        _actionTime -= timeElapsed;
    else {
        if (_actionTarget) {
            server.gatherObject(_actionTarget->serial(), *this);
            _actionTarget = 0;
        } else if (_actionCrafting) {
            // Give user his newly crafted item
            const size_t slot = giveItem(_actionCrafting);
            if (slot == INVENTORY_SIZE) {
                server.sendMessage(_socket, SV_INVENTORY_FULL);
                _actionCrafting = 0;
                return;
            }
            server.sendMessage(_socket, SV_INVENTORY, makeArgs(slot, _actionCrafting->id(), 1));
            // Remove materials from user's inventory
            removeMaterials(*_actionCrafting, server);
            _actionCrafting = 0;
        }
        _actionTime = 0;
        server.sendMessage(_socket, SV_ACTION_FINISHED);
    }
}
