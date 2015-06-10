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
location(loc),
_socket(socket),
inventory(INVENTORY_SIZE, std::make_pair("none", 0)),
_lastLocUpdate(SDL_GetTicks()),
_lastContact(SDL_GetTicks()){}

User::User(const Socket &rhs):
_socket(rhs){}

bool User::operator<(const User &rhs) const{
    return _socket < rhs._socket;
}

const std::string &User::getName() const{
    return _name;
}

const Socket &User::getSocket() const{
    return _socket;
}

std::string User::makeLocationCommand() const{
    return makeArgs(_name, location.x, location.y);
}

void User::updateLocation(const Point &dest){
    Uint32 newTime = SDL_GetTicks();
    Uint32 timeElapsed = newTime - _lastLocUpdate;
    _lastLocUpdate = newTime;

    // Max legal distance: straight line
    double maxLegalDistance = timeElapsed / 1000.0 * Client::MOVEMENT_SPEED;
    location = interpolate(location, dest, maxLegalDistance);
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
        if (inventory[i].first == item.id && inventory[i].second < item.stackSize){
            ++inventory[i].second;
            return i;
        }
        if (inventory[i].first == "none" && emptySlot == INVENTORY_SIZE)
            emptySlot = i;
    }
    if (emptySlot != INVENTORY_SIZE) {
        inventory[emptySlot] = std::make_pair(item.id, 1);
    }
    return emptySlot;
}
