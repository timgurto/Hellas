#include <sstream>

#include "Client.h"
#include "Server.h"
#include "Socket.h"
#include "User.h"
#include "messageCodes.h"
#include "util.h"

const size_t User::INVENTORY_SIZE = 5;

User::User(const std::string &name, const Point &loc, const Socket &socket):
_name(name),
_location(loc),
_actionTarget(0),
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
    double maxLegalDistance = timeElapsed / 1000.0 * Client::MOVEMENT_SPEED * 3;
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

void User::actionTarget(const BranchLite *branch){
    _actionTarget = branch;
    _actionTime = BranchLite::ACTION_TIME;
}

void User::update(Uint32 timeElapsed, Server &server){
    if (_actionTime == 0)
        return;
    if (_actionTime > timeElapsed)
        _actionTime -= timeElapsed;
    else {
        server.removeBranch(_actionTarget->serial, *this);
        _actionTime = 0;
    }
}
