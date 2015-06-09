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

    Uint32 latency;

    void contact();
    bool alive() const; // Whether the client has contacted the server recently enough

    // Determine whether the proposed new location is legal, considering movement speed and time elapsed.
    // Set location to the new, legal location
    void updateLocation(const Point &dest);

private:
    std::string _name;
    Socket _socket;
    Uint32 _lastLocUpdate; // Time that the last CL_LOCATION was received
    Uint32 _lastContact;
};

#endif
