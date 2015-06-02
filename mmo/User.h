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
    // Multiplier for legal movement distance, to compensate for latency fluctuations
    // TODO: consider ability for users to exploit this
    static const double LEGAL_MOVEMENT_MARGIN;

    Uint32 latency;

    // Determine whether the proposed new location is legal, considering movement speed and time elapsed.
    // Set location to the new, legal location
    void updateLocation(double x, double y);

private:
    std::string _name;
    Socket _socket;
    Uint32 _lastLocUpdate; // Time that the last CL_LOCATION was received
};

#endif
