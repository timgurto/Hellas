// (C) 2015 Tim Gurto

#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>
#include <windows.h>

#include "Item.h"
#include "Object.h"
#include "Recipe.h"
#include "../Point.h"
#include "../Socket.h"

class Server;

// Stores information about a single user account for the server
class User{
    std::string _name;
    Socket _socket;
    Point _location;
    const Object *_actionTarget; // Points to the object that the user is acting upon.
    const Recipe *_actionCrafting; // The recipe this user is currently crafting.

    const ObjectType *_actionConstructing; // The object this user is currently constructing.
    size_t _constructingSlot;
    Point _constructingLocation;

    Uint32 _actionTime; // Time remaining on current action.
    Item::vect_t _inventory;

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
    const std::pair<const Item *, size_t> &inventory(size_t index) const;
    std::pair<const Item *, size_t> &inventory(size_t index);

    const Rect collisionRect() const;

    const Object *actionTarget() const { return _actionTarget; }
    void actionTarget(const Object *object); // Configure user to perform an action on an object

    // Whether the user has enough materials to craft a recipe
    bool hasItems(const ItemSet &items) const;
    void removeItems(const ItemSet &items, Server &server);
    bool hasTool(const std::string &className) const;
    bool hasTools(const std::set<std::string> &classes) const;
    void actionCraft(const Recipe &item); // Configure user to craft an item

    // Configure user to construct an item
    void actionConstruct(const ObjectType &obj, const Point &location, size_t slot);

    void cancelAction(Server &server); // Cancel any action in progress, and alert the client

    std::string makeLocationCommand() const;

    static const size_t INVENTORY_SIZE;

    static const ObjectType OBJECT_TYPE;

    void contact();
    bool alive() const; // Whether the client has contacted the server recently enough

    /*
    Determine whether the proposed new location is legal, considering movement speed and
    time elapsed, and checking for collisions.
    Set location to the new, legal location.
    */
    void updateLocation(const Point &dest, Server &server);

    // Whether the user has at least one item with the specified item class
    bool hasItemClass(const std::string &className) const;

    // Return value: 0 if there was room for all items, otherwise the remainder.
    size_t giveItem(const Item *item, size_t quantity, const Server &server);

    // Whether the user has room for one or more of an item
    bool hasSpace(const Item *item, size_t quantity = 1) const;

    void update(Uint32 timeElapsed, Server &server);
};

#endif
