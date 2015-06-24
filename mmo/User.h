#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>
#include <windows.h>

#include "Point.h"
#include "Socket.h"

class Server;

// Stores information about a single user account for the server
class User{
    std::string _name;
    Socket _socket;
    Point _location;
    std::vector<std::pair<std::string, size_t> > _inventory;

    Uint32 _lastLocUpdate; // Time that the last CL_LOCATION was received
    Uint32 _lastContact;
    Uint32 _latency;

public:
    User(const std::string &name, const Point &loc, const Socket &socket);
    User(const Socket &rhs); // for use with set::find(), allowing find-by-socket

    inline bool operator<(const User &rhs) const { return _socket < rhs._socket; }

    inline const std::string &name() const { return _name; }
    inline const Socket &socket() const { return _socket; }
    inline const Point &location() const { return _location; }
    inline void location(const Point &loc) { _location = loc; }
    void location(std::istream &is); // Read co-ordinates from stream
    const std::pair<std::string, size_t> &inventory(size_t index) const;
    std::pair<std::string, size_t> &inventory(size_t index);

    std::string makeLocationCommand() const;

    static const size_t INVENTORY_SIZE;

    void contact();
    bool alive() const; // Whether the client has contacted the server recently enough

    // Determine whether the proposed new location is legal, considering movement speed and time elapsed.
    // Set location to the new, legal location
    void updateLocation(const Point &dest, const Server &server);

    // Return value: which inventory slot was used
    size_t giveItem(const Item &item);
};

#endif
