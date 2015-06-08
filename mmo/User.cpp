#include <sstream>

#include "Client.h"
#include "Socket.h"
#include "User.h"
#include "messageCodes.h"
#include "util.h"

User::User(const std::string &name, const Point &loc, const Socket &socket):
_name(name),
location(loc),
_socket(socket),
_lastLocUpdate(SDL_GetTicks()){}

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
