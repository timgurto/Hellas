// (C) 2015 Tim Gurto

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
    const BranchLite *_actionTargetBranch; // Points to a branch that the user is acting upon.
    const TreeLite *_actionTargetTree; // Points to a branch that the user is acting upon.
    const Item *_actionCrafting; // The item this user is currently crafting.
    Uint32 _actionTime; // Time remaining on current action.
    std::vector<std::pair<std::string, size_t> > _inventory;

    Uint32 _lastLocUpdate; // Time that the last CL_LOCATION was received
    Uint32 _lastContact;
    Uint32 _latency;

public:
    User(const std::string &name, const Point &loc, const Socket &socket);
    User(const Socket &rhs); // for use with set::find(), allowing find-by-socket

    bool operator<(const User &rhs) const { return _socket < rhs._socket; }

    const std::string &name() const { return _name; }
    const Socket &socket() const { return _socket; }
    const Point &location() const { return _location; }
    void location(const Point &loc) { _location = loc; }
    void location(std::istream &is); // Read co-ordinates from stream
    const std::pair<std::string, size_t> &inventory(size_t index) const;
    std::pair<std::string, size_t> &inventory(size_t index);

    const BranchLite *actionTargetBranch() const { return _actionTargetBranch; }
    const TreeLite *actionTargetTree() const { return _actionTargetTree; }
    void actionTargetBranch(const BranchLite *branch); // Configure user to gather a branch
    void actionTargetTree(const TreeLite *tree); // Configure user to gather a tree

    bool hasMaterials(const Item &item) const; // Whether the user has enough materials to craft an item
    void removeMaterials(const Item &item, Server &server);
    void actionCraft(const Item &item); // Configure user to craft an item

    std::string makeLocationCommand() const;

    static const size_t INVENTORY_SIZE;

    void contact();
    bool alive() const; // Whether the client has contacted the server recently enough

    // Determine whether the proposed new location is legal, considering movement speed and time elapsed.
    // Set location to the new, legal location
    void updateLocation(const Point &dest, const Server &server);

    // Return value: which inventory slot was used
    size_t giveItem(const Item &item);

    void update(Uint32 timeElapsed, Server &server);
};

#endif
