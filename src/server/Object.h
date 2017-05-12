#ifndef OBJECT_H
#define OBJECT_H

#include "MerchantSlot.h"
#include "Container.h"
#include "Deconstruction.h"
#include "ObjectType.h"
#include "Permissions.h"
#include "ItemSet.h"
#include "../Point.h"

class Spawner;
class User;

// A server-side representation of an in-game object
class Object{
    size_t _serial;
    Point _location;
    const ObjectType *_type;
    Permissions _permissions;
    ItemSet _contents; // Remaining contents, which can be gathered
    std::vector<MerchantSlot> _merchantSlots;

    Spawner *_spawner; // The Spawner that created this object, if any.

    size_t _numUsersGathering; // The number of users gathering from this object.

    ms_t _lastLocUpdate; // Time that the location was last updated; used to determine max distance

    // Users watching this object for changes to inventory or merchant slots
    std::set<std::string> _watchers;

    ItemSet _remainingMaterials; // The remaining construction costs, if relevant.

    ms_t _transformTimer; // When this hits zero, it switches types.

protected:
    static size_t generateSerial();

public:
    Object(const ObjectType *type, const Point &loc); // Generates a new serial

    Object(){} // For lookup dummies only.
    Object(size_t serial); // For set/map lookup; contains only a serial
    Object(const Point &loc); // For set/map lookup; contains only a location

    virtual ~Object(){}

    const Point &location() const { return _location; }
    void location(const Point &loc) { _location = loc; }
    size_t serial() const { return _serial; }
    void serial(size_t s) { _serial = s; }
    const ObjectType *type() const { return _type; }
    const ItemSet &contents() const { return _contents; }
    void contents(const ItemSet &contents);
    const Rect collisionRect() const { return _type->collisionRect() + _location;  }
    const std::vector<MerchantSlot> &merchantSlots() const { return _merchantSlots; }
    const MerchantSlot &merchantSlot(size_t slot) const { return _merchantSlots[slot]; }
    MerchantSlot &merchantSlot(size_t slot) { return _merchantSlots[slot]; }
    Spawner *spawner() const { return _spawner; }
    void spawner(Spawner *p) { _spawner = p; }
    const std::set<std::string> &watchers() const { return _watchers; }
    void incrementGatheringUsers(const User *userToSkip = nullptr);
    void decrementGatheringUsers(const User *userToSkip = nullptr);
    void removeAllGatheringUsers();
    size_t numUsersGathering() const { return _numUsersGathering; }
    bool isBeingBuilt() const { return !_remainingMaterials.isEmpty(); }
    const ItemSet &remainingMaterials() const { return _remainingMaterials; }
    ItemSet &remainingMaterials() { return _remainingMaterials; }
    void removeMaterial(const ServerItem *item, size_t qty);
    bool isTransforming() const { return _transformTimer > 0; }
    ms_t transformTimer() const { return _transformTimer; }
    Permissions &permissions() { return _permissions; }
    const Permissions &permissions() const { return _permissions; }

    bool hasContainer() const { return _container != nullptr; }
    Container &container() { return *_container; }
    const Container &container() const { return *_container; }

    bool hasDeconstruction() const { return _deconstruction != nullptr; }
    Deconstruction &deconstruction() { return *_deconstruction; }
    const Deconstruction &deconstruction() const { return *_deconstruction; }

    bool isAbleToDeconstruct(const User &user) const;

    virtual bool collides() const { return _type->collides(); }
    virtual double speed() const { return 0; } // movement per second

    virtual char classTag() const { return 'o'; }

    virtual void update(ms_t timeElapsed);
    // Add this object to a list, for removal after all objects are updated.
    void markForRemoval();

    void setType(const ObjectType *type); // Set/change ObjectType

    virtual void onRemove();

    /*
    Determine whether the proposed new location is legal, considering movement speed and
    time elapsed, and checking for collisions.
    Set location to the new, legal location.
    */
    void updateLocation(const Point &dest);

    // Randomly choose an item type for the user to gather.
    const ServerItem *chooseGatherItem() const;
    // Randomly choose a quantity of the above items, between 1 and the object's contents.
    size_t chooseGatherQuantity(const ServerItem *item) const;
    void removeItem(const ServerItem *item, size_t qty); // From _contents; gathering
    
    void addWatcher(const std::string &username);
    void removeWatcher(const std::string &username);
    
    struct compareSerial{ bool operator()(const Object *a, const Object *b); };
    struct compareXThenSerial{ bool operator()( const Object *a, const Object *b); };
    struct compareYThenSerial{ bool operator()( const Object *a, const Object *b); };
    typedef std::set<const Object*, Object::compareXThenSerial> byX_t;
    typedef std::set<const Object*, Object::compareYThenSerial> byY_t;

    friend class Container; // TODO: Remove once everything is componentized

private:
    Container *_container;
    Deconstruction *_deconstruction;
};

#endif
