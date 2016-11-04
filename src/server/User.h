#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>
#include <windows.h>

#include "Combatant.h"
#include "Object.h"
#include "Recipe.h"
#include "ServerItem.h"
#include "../Point.h"
#include "../Socket.h"

class NPC;
class Server;

// Stores information about a single user account for the server
class User : public Combatant{
public:
    enum Action{
        GATHER,
        CRAFT,
        CONSTRUCT,
        DECONSTRUCT,
        ATTACK,

        NO_ACTION
    };

    // Stats
    static const health_t
        MAX_HEALTH,
        ATTACK_DAMAGE;
    static const ms_t
        ATTACK_TIME;
    static const double
        MOVEMENT_SPEED;

private:
    std::string _name;
    Socket _socket;

    Action _action;
    ms_t _actionTime; // Time remaining on current action.
    // Information used when action completes:
    Object *_actionObject; // Gather, deconstruct
    const Recipe *_actionRecipe; // Craft
    const ObjectType *_actionObjectType; // Construct
    size_t _actionSlot; // Construct
    Point _actionLocation; // Construct

    ServerItem::vect_t _inventory, _gear;

    ms_t _lastContact;
    ms_t _latency;


public:
    User(const std::string &name, const Point &loc, const Socket &socket);
    User(const Socket &rhs); // for use with set::find(), allowing find-by-socket
    User(const Point &loc); // for use with set::find(), allowing find-by-location

    bool operator<(const User &rhs) const { return _socket < rhs._socket; }

    const std::string &name() const { return _name; }
    const Socket &socket() const { return _socket; }

    // Inventory
    const std::pair<const ServerItem *, size_t> &inventory(size_t index) const
            { return _inventory[index]; }
    std::pair<const ServerItem *, size_t> &inventory(size_t index)
            { return _inventory[index]; }
    ServerItem::vect_t &inventory() { return _inventory; }
    const ServerItem::vect_t &inventory() const { return _inventory; }

    // Gear
    const std::pair<const ServerItem *, size_t> &gear(size_t index) const
            { return _gear[index]; }
    std::pair<const ServerItem *, size_t> &gear(size_t index)
            { return _gear[index]; }
    ServerItem::vect_t &gear() { return _gear; }
    const ServerItem::vect_t &gear() const { return _gear; }

    virtual health_t maxHealth() const override { return MAX_HEALTH; }
    virtual health_t attack() const override { return ATTACK_DAMAGE; }
    virtual ms_t attackTime() const override { return ATTACK_TIME; }
    virtual double speed() const override { return MOVEMENT_SPEED; }

    virtual char classTag() const override { return 'u'; }
    
    virtual void onHealthChange() override;
    virtual void onDeath() override;

    const Rect collisionRect() const;

    Action action() const { return _action; }
    void action(Action a) { _action = a; }
    const Object *actionObject() const { return _actionObject; }
    void beginGathering(Object *object); // Configure user to perform an action on an object
    void targetNPC(NPC *npc); // Configure user to prepare to attack an NPC

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
    void finishAction(); // An action has just ended; clean up the state.

    std::string makeLocationCommand() const;
    
    static const size_t INVENTORY_SIZE;
    static const size_t GEAR_SLOTS;

    static ObjectType OBJECT_TYPE;

    void contact();
    bool alive() const; // Whether the client has contacted the server recently enough

    // Return value: 0 if there was room for all items, otherwise the remainder.
    size_t giveItem(const ServerItem *item, size_t quantity = 1);

    void update(ms_t timeElapsed);

    struct compareXThenSerial{ bool operator()( const User *a, const User *b); };
    struct compareYThenSerial{ bool operator()( const User *a, const User *b); };
    typedef std::set<const User*, User::compareXThenSerial> byX_t;
    typedef std::set<const User*, User::compareYThenSerial> byY_t;

    void registerLocUpdate();
};

#endif
