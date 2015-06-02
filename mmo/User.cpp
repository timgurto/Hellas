#include <sstream>

#include "Socket.h"
#include "User.h"
#include "messageCodes.h"

User::User(const std::string &name, const Point &loc, const Socket &socket):
_name(name),
location(loc),
_socket(socket){}

User::User(const Socket &rhs):
location(0, 0),
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
    std::ostringstream oss;
    oss << '[' << SV_LOCATION << ',' << _name << ',' << location.x << ',' << location.y << ']';
    return oss.str();
}
