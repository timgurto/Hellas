#ifndef ENTITY_H
#define ENTITY_H

#include "EntityType.h"
#include "ServerItem.h"
#include "../Message.h"
#include "../Point.h"
#include "../Rect.h"
#include "../types.h"

class Spawner;
class User;
class XmlWriter;

// Abstract class describing location, movement and combat functions of something in the game world
class Entity {

public:
    Entity(const EntityType *type, const Point &loc, health_t health);
    Entity(size_t serial); // TODO make private
    Entity(const Point &loc); // TODO make private
    virtual ~Entity();
    
    const EntityType *type() const { return _type; }

    virtual char classTag() const = 0;
    
    struct compareSerial{ bool operator()(const Entity *a, const Entity *b); };
    struct compareXThenSerial{ bool operator()( const Entity *a, const Entity *b); };
    struct compareYThenSerial{ bool operator()( const Entity *a, const Entity *b); };
    typedef std::set<const Entity*, Entity::compareXThenSerial> byX_t;
    typedef std::set<const Entity*, Entity::compareYThenSerial> byY_t;

    size_t serial() const { return _serial; }
    void serial(size_t s) { _serial = s; }

    virtual void update(ms_t timeElapsed);
    // Add this entity to a list, for removal after all objects are updated.
    void markForRemoval();

    virtual void sendInfoToClient(const User &targetUser) const = 0;

    virtual void writeToXML(XmlWriter &xw) const {}

    Spawner *spawner() const { return _spawner; }
    void spawner(Spawner *p) { _spawner = p; }
    
    const std::set<std::string> &watchers() const { return _watchers; }
    void addWatcher(const std::string &username);
    void removeWatcher(const std::string &username);
    
    // Space
    const Point &location() const { return _location; }
    void location(const Point &loc) { _location = loc; }
    const Rect collisionRect() const { return type()->collisionRect() + _location; }
    bool collides() const { return type()->collides() && _health != 0; }
    virtual double speed() const { return 0; } // movement per second


    // Combat
    virtual health_t maxHealth() const = 0;
    virtual health_t attack() const = 0;
    virtual ms_t attackTime() const = 0;
    Entity *target() const { return _target; }
    void target(Entity *p) { _target = p; }
    virtual ms_t timeToRemainAsCorpse() const = 0;

    health_t health() const { return _health; }
    void health(health_t health) { _health = health; }

    void reduceHealth(int damage);
    virtual void onHealthChange() {}; // Probably alerting relevant users.
    virtual void onDeath(); // Anything that needs to happen upon death.
    virtual void describeSelfToNewWatcher(const User &watcher) const {}
    virtual void alertWatcherOnInventoryChange(const User &watcher, size_t slot) const {}
    virtual ServerItem::Slot *getSlotToTakeFromAndSendErrors(size_t slotNum, const User &user) { return nullptr; }
    virtual void onOutOfRange(const Entity &rhs) const {} // This will be called for both entities.
    virtual Message outOfRangeMessage() const { return Message(); };
    virtual bool shouldAlwaysBeKnownToUser(const User &user) const { return false; }

    /*
    Determine whether the proposed new location is legal, considering movement speed and
    time elapsed, and checking for collisions.
    Set location to the new, legal location.
    */
    void updateLocation(const Point &dest);

protected:
    void type(const EntityType *type) { _type = type; }

private:
    static size_t generateSerial();
    const EntityType *_type;
    
    Spawner *_spawner; // The Spawner that created this entity, if any.

    // Users watching this object for changes to inventory or merchant slots
    std::set<std::string> _watchers;

    // Space
    size_t _serial;
    Point _location;
    ms_t _lastLocUpdate; // Time that the location was last updated; used to determine max distance


    // Combat
    health_t _health;
    ms_t _attackTimer;
    Entity *_target;
    ms_t _corpseTime; // How much longer this entity should exist as a corpse.
    void startCorpseTimer();


    friend class Dummy;
};


class Dummy : public Entity{
public:
    static Dummy Serial(size_t serial) { return Dummy(serial); }
    static Dummy Location(const Point &loc) { return Dummy(loc); }
    static Dummy Location(double x, double y) { return Dummy(Point(x, y)); }
private:
    friend class Entity;
    Dummy(size_t serial) : Entity(serial) {}
    Dummy(const Point &loc) : Entity(loc) {}

    // Necessary overrides to make this a concrete class
    virtual char classTag() const override { return 'd'; }
    virtual health_t maxHealth() const override { return 0; };
    virtual health_t attack() const override { return 0; };
    virtual ms_t attackTime() const override { return 0; };
    virtual void sendInfoToClient(const User &targetUser) const override {}
    virtual ms_t timeToRemainAsCorpse() const override { return 0; }
};

#endif
