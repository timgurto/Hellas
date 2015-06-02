#ifndef USER_H
#define USER_H

#include <string>
#include <windows.h>

#include "Point.h"
#include "Socket.h"

// Stores information about a single user account for the server
class User{
public:
    User(const std::string &name, const Point &loc, const Socket &socket);
    User(const Socket &rhs); // for use with set::find(), allowing find-by-socket

    bool operator<(const User &rhs) const;

    const std::string &getName() const;
    const Socket &getSocket() const;

    std::string makeLocationCommand() const;

    Point location;

private:
    std::string _name;
    Socket _socket;
};

#endif
