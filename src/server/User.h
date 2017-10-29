#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>
#include <windows.h>

#include "City.h"
#include "Entity.h"
#include "Recipe.h"
#include "ServerItem.h"
#include "objects/Object.h"
#include "../Point.h"
#include "../Socket.h"
#include "../Stats.h"

class NPC;
class Server;

// Stores information about a single user account for the server
class User : public Object{
public:
    enum Action{
        GATHER,
        CRAFT,
        CONSTRUCT,
        DECONSTRUCT,
        ATTACK,

        NO_ACTION
    };

    enum Class{
        SOLDIER,
        MAGUS,
        PRIEST,

        NUM_CLASSES
    };
    static std::map<Class, std::string> CLASS_NAMES;
    static std::map<std::string, Class> CLASS_CODES;

    static Stats BASE_STATS;

private:
    std::string _name;
    Socket _socket;

    Class _class;

    Action _action;
    ms_t _actionTime; // Time remaining on current action.
    // Information used when action completes:
    Object *_actionObject; // Gather, deconstruct
    const Recipe *_actionRecipe; // Craft
    const ObjectType *_actionObjectType; // Construct
    size_t _actionSlot; // Construct
    Point _actionLocation; // Construct

    std::set<std::string>
        _knownRecipes,
        _knownConstructions;
    mutable std::set<std::string> _playerUniqueCategoriesOwned;

    size_t _driving; // The serial of the vehicle this user is currently driving; 0 if none.

    Stats _stats; // Memoized stats, after gear etc.  Calculated with updateStats();

    ServerItem::vect_t _inventory, _gear;

    ms_t _lastContact;
    ms_t _latency;

    Point _respawnPoint;


public:
    User(const std::string &name, const Point &loc, const Socket &socket);
    User(const Socket &rhs); // for use with set::find(), allowing find-by-socket
    User(const Point &loc); // for use with set::find(), allowing find-by-location
    virtual ~User(){}

    bool operator<(const User &rhs) const { return _socket < rhs._socket; }

    const std::string &name() const { return _name; }
    const Socket &socket() const { return _socket; }
    void setClass(Class c) { _class = c; }
    Class getClass() const { return _class; }
    const std::string &className() const { return CLASS_NAMES[_class]; }
    size_t driving() const { return _driving; }
    void driving(size_t serial) { _driving = serial; }
    bool isDriving() const { return _driving != 0; }
    const std::set<std::string> &knownRecipes() const { return _knownRecipes; }
    void addRecipe(const std::string &id) { _knownRecipes.insert(id); }
    bool knowsRecipe(const std::string &id) const;
    const std::set<std::string> &knownConstructions() const { return _knownConstructions; }
    void addConstruction(const std::string &id) { _knownConstructions.insert(id); }
    bool knowsConstruction(const std::string &id) const;
    bool hasRoomToCraft(const Recipe &recipe) const;
    bool hasPlayerUnique(const std::string &category) const {
        return _playerUniqueCategoriesOwned.find(category) != _playerUniqueCategoriesOwned.end();
    }
    const Point &respawnPoint() const { return _respawnPoint; }
    void respawnPoint(const Point &loc) { _respawnPoint = loc; }

    // Inventory getters/setters
    const std::pair<const ServerItem *, size_t> &inventory(size_t index) const
            { return _inventory[index]; }
    std::pair<const ServerItem *, size_t> &inventory(size_t index)
            { return _inventory[index]; }
    ServerItem::vect_t &inventory() { return _inventory; }
    const ServerItem::vect_t &inventory() const { return _inventory; }

    // Gear getters/setters
    const std::pair<const ServerItem *, size_t> &gear(size_t index) const
            { return _gear[index]; }
    std::pair<const ServerItem *, size_t> &gear(size_t index)
            { return _gear[index]; }
    ServerItem::vect_t &gear() { return _gear; }
    const ServerItem::vect_t &gear() const { return _gear; }

    static void init();

    void updateStats();
    Hitpoints maxHealth() const override { return _stats.health; }
    Energy maxEnergy() const override { return _stats.energy; }
    Hitpoints attack() const override { return _stats.attack; }
    ms_t attackTime() const override { return _stats.attackTime; }
    double speed() const override { return _stats.speed; }
    ms_t timeToRemainAsCorpse() const override { return 0; }

    char classTag() const override { return 'u'; }

    void onHealthChange() override;
    void onEnergyChange() override;
    void onDeath() override;
    void onNewOwnedObject(const ObjectType &type) const;

    void sendInfoToClient(const User &targetUser) const override;

    void onOutOfRange(const Entity &rhs) const override;
    Message outOfRangeMessage() const override;

    const Rect collisionRect() const;

    Action action() const { return _action; }
    void action(Action a) { _action = a; }
    const Object *actionObject() const { return _actionObject; }
    void beginGathering(Object *object); // Configure user to perform an action on an object
    void setTargetAndAttack(Entity *target); // Configure user to prepare to attack an NPC or player

    // Whether the user has enough materials to craft a recipe
    bool hasItems(const ItemSet &items) const;
    void removeItems(const ItemSet &items);
    bool hasTool(const std::string &tagName) const;
    bool hasTools(const std::set<std::string> &classes) const;

    void beginCrafting(const Recipe &item); // Configure user to craft an item

    // Configure user to construct an item, or an object from no item
    void beginConstructing(const ObjectType &obj, const Point &location,
                           size_t slot = INVENTORY_SIZE);

    // Configure user to deconstruct an object
    void beginDeconstructing(Object &obj);

    void cancelAction(); // Cancel any action in progress, and alert the client
    void finishAction(); // An action has just ended; clean up the state.

    std::string makeLocationCommand() const;
    
    static const size_t INVENTORY_SIZE = 10;
    static const size_t GEAR_SLOTS = 8;

    static ObjectType OBJECT_TYPE;

    void contact();
    bool alive() const; // Whether the client has contacted the server recently enough

    // Return value: 0 if there was room for all items, otherwise the remainder.
    size_t giveItem(const ServerItem *item, size_t quantity = 1);

    void update(ms_t timeElapsed);

    static Point newPlayerSpawn;
    static double spawnRadius;
    void moveToSpawnPoint(bool isNewPlayer = false);

    void broadcastHealth() const override;

    struct compareXThenSerial{ bool operator()( const User *a, const User *b) const; };
    struct compareYThenSerial{ bool operator()( const User *a, const User *b) const; };
    typedef std::set<const User*, User::compareXThenSerial> byX_t;
    typedef std::set<const User*, User::compareYThenSerial> byY_t;
};

#endif
