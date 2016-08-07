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
public:
    enum Action{
        GATHER,
        CRAFT,
        CONSTRUCT,
        DECONSTRUCT,

        NO_ACTION
    };

private:
    std::string _name;
    Socket _socket;
    Point _location;

    Action _action;
    ms_t _actionTime; // Time remaining on current action.
    // Information used when action completes:
    Object *_actionObject; // Gather, deconstruct
    const Recipe *_actionRecipe; // Craft
    const ObjectType *_actionObjectType; // Construct
    size_t _actionSlot; // Construct
    Point _actionLocation; // Construct

    Item::vect_t _inventory;

    ms_t _lastLocUpdate; // Time that the last CL_LOCATION was received
    ms_t _lastContact;
    ms_t _latency;

    // Stats
    static const unsigned
        ATTACK,
        MAX_HEALTH;
    unsigned
        _health;

public:
    User(const std::string &name, const Point &loc, const Socket &socket);
    User(const Socket &rhs); // for use with set::find(), allowing find-by-socket
    User(const Point &loc); // for use with set::find(), allowing find-by-location

    bool operator<(const User &rhs) const { return _socket < rhs._socket; }

    const std::string &name() const { return _name; }
    const Socket &socket() const { return _socket; }
    const Point &location() const { return _location; }
    void location(const Point &loc) { _location = loc; }
    const std::pair<const Item *, size_t> &inventory(size_t index) const;
    std::pair<const Item *, size_t> &inventory(size_t index);
    Item::vect_t &inventory() { return _inventory; }
    const Item::vect_t &inventory() const { return _inventory; }

    const Rect collisionRect() const;

    Action action() const { return _action; }
    void action(Action a) { _action = a; }
    const Object *actionObject() const { return _actionObject; }
    void beginGathering(Object *object); // Configure user to perform an action on an object

    // Whether the user has enough materials to craft a recipe
    bool hasItems(const ItemSet &items) const;
    void removeItems(const ItemSet &items);
    bool hasTool(const std::string &className) const;
    bool hasTools(const std::set<std::string> &classes) const;

    void beginCrafting(const Recipe &item); // Configure user to craft an item

    // Configure user to construct an item
    void beginConstructing(const ObjectType &obj, const Point &location, size_t slot);

    // Configure user to deconstruct an object
    void beginDeconstructing(Object &obj);

    void cancelAction(); // Cancel any action in progress, and alert the client

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
    void updateLocation(const Point &dest);

    // Return value: 0 if there was room for all items, otherwise the remainder.
    size_t giveItem(const Item *item, size_t quantity = 1);

    void update(ms_t timeElapsed);

    struct compareXThenSerial{ bool operator()( const User *a, const User *b); };
    struct compareYThenSerial{ bool operator()( const User *a, const User *b); };
    typedef std::set<const User*, User::compareXThenSerial> byX_t;
    typedef std::set<const User*, User::compareYThenSerial> byY_t;
};

#endif
