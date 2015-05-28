#include <sstream>

#include "User.h"
#include "messageCodes.h"

User::User(const std::string &name, const std::pair<int, int> &loc, SOCKET socket):
_name(name),
location(loc),
_socket(socket){}

const std::string &User::getName() const{
    return _name;
}

SOCKET User::getSocket() const{
    return _socket;
}

std::string User::makeLocationCommand() const{
    std::ostringstream oss;
    oss << '[' << SV_LOCATION << ',' << _name << ',' << location.first << ',' << location.second << ']';
    return oss.str();
}
