// (C) 2015 Tim Gurto

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
_location(loc),
_actionTargetBranch(0),
_actionTargetTree(0),
_actionCrafting(0),
_actionTime(0),
_socket(socket),
_inventory(INVENTORY_SIZE, std::make_pair("none", 0)),
_lastLocUpdate(SDL_GetTicks()),
_lastContact(SDL_GetTicks()){}

User::User(const Socket &rhs):
_socket(rhs){}

void User::location(std::istream &is){
    is >> _location.x >> _location.y;
}

const std::pair<std::string, size_t> &User::inventory(size_t index) const{
    return _inventory[index];
}

std::pair<std::string, size_t> &User::inventory(size_t index){
    return _inventory[index];
}

std::string User::makeLocationCommand() const{
    return makeArgs(_name, _location.x, _location.y);
}

void User::updateLocation(const Point &dest, const Server &server){
    Uint32 newTime = SDL_GetTicks();
    Uint32 timeElapsed = newTime - _lastLocUpdate;
    _lastLocUpdate = newTime;

    // Max legal distance: straight line
    double maxLegalDistance = (min(Server::MAX_TIME_BETWEEN_LOCATION_UPDATES, timeElapsed + 100))
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

size_t User::giveItem(const Item &item){
    size_t emptySlot = INVENTORY_SIZE;
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first == item.id() && _inventory[i].second < item.stackSize()){
            ++_inventory[i].second;
            return i;
        }
        if (_inventory[i].first == "none" && emptySlot == INVENTORY_SIZE)
            emptySlot = i;
    }
    if (emptySlot != INVENTORY_SIZE) {
        _inventory[emptySlot] = std::make_pair(item.id(), 1);
    }
    return emptySlot;
}

void User::cancelAction(Server &server) {
    if (_actionTargetBranch || _actionTargetTree || _actionCrafting)
        server.sendMessage(_socket, SV_ACTION_INTERRUPTED);
    _actionTargetBranch = 0;
    _actionTargetTree = 0;
    _actionCrafting = 0;
    _actionTime = 0;
}

void User::actionTargetBranch(const BranchLite *branch){
    _actionTargetBranch = branch;
    _actionTime = BranchLite::ACTION_TIME;
}

void User::actionTargetTree(const TreeLite *tree){
    _actionTargetTree = tree;
    _actionTime = TreeLite::ACTION_TIME;
}

void User::actionCraft(const Item &item){
    _actionCrafting = &item;
    _actionTime = item.craftTime();
}

bool User::hasMaterials(const Item &item) const{
    std::map<std::string, size_t> remaining = item.materials();
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        const std::pair<std::string, size_t> &invSlot = _inventory[i];
        std::map<std::string, size_t>::iterator it = remaining.find(invSlot.first);
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

void User::removeMaterials(const Item &item, Server &server) {
    std::set<size_t> invSlotsChanged;
    std::map<std::string, size_t> remaining = item.materials();
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        std::pair<std::string, size_t> &invSlot = _inventory[i];
        std::map<std::string, size_t>::iterator it = remaining.find(invSlot.first);
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
                invSlot.first = "none";
                invSlotsChanged.insert(i);
            }
        }
    }
    for (std::set<size_t>::const_iterator it = invSlotsChanged.begin(); it != invSlotsChanged.end(); ++it){
        const std::pair<std::string, size_t> &slot = _inventory[*it];
        server.sendMessage(_socket, SV_INVENTORY, makeArgs(*it, slot.first, slot.second));
    }
}

void User::update(Uint32 timeElapsed, Server &server){
    if (_actionTime == 0)
        return;
    if (_actionTime > timeElapsed)
        _actionTime -= timeElapsed;
    else {
        if (_actionTargetBranch) {
            server.removeBranch(_actionTargetBranch->serial, *this);
            _actionTargetBranch = 0;
        } else if (_actionTargetTree) {
            server.removeTree(_actionTargetTree->serial, *this);
            _actionTargetTree = 0;
        } else if (_actionCrafting) {
            // Give user his newly crafted item
            size_t slot = giveItem(*_actionCrafting);
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
