#ifndef ENTITY_H
#define ENTITY_H

#include <cassert>
#include <memory>

#include "Buff.h"
#include "EntityType.h"
#include "Loot.h"
#include "ServerItem.h"
#include "ThreatTable.h"
#include "../Message.h"
#include "../Point.h"
#include "../Rect.h"
#include "../SpellSchool.h"
#include "../types.h"

class Spawner;
class User;
class XmlWriter;

// Abstract class describing location, movement and combat functions of something in the game world
class Entity {

public:
    Entity(const EntityType *type, const MapPoint &loc);
    Entity(size_t serial); // TODO make private
    Entity(const MapPoint &loc); // TODO make private
    virtual ~Entity();
    
    const EntityType *type() const { return _type; }

    virtual char classTag() const = 0;
    
    struct compareSerial{ bool operator()(const Entity *a, const Entity *b) const; };
    struct compareXThenSerial{ bool operator()( const Entity *a, const Entity *b) const; };
    struct compareYThenSerial{ bool operator()( const Entity *a, const Entity *b) const; };
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
    const MapPoint &location() const { return _location; }
    void location(const MapPoint &loc, bool firstInsertion = false);
    void changeDummyLocation(const MapPoint &loc) { _location = loc; }
    const MapRect collisionRect() const { return type()->collisionRect() + _location; }
    bool collides() const { return type()->collides() && _health != 0; }


    // Combat
    Entity *target() const { return _target; }
    void target(Entity *p) { _target = p; }
    virtual void updateStats() {} // Recalculate _stats based on any modifiers
    virtual ms_t timeToRemainAsCorpse() const = 0;
    ms_t corpseTime() const { return _corpseTime; }
    void corpseTime(ms_t time) { _corpseTime = time; }
    virtual bool canBeAttackedBy(const User &user) const = 0;
    virtual px_t attackRange() const { return MELEE_RANGE; }
    virtual CombatResult generateHitAgainst(const Entity &target, CombatType type, SpellSchool school, px_t range) const { return FAIL; }
        static bool combatTypeCanHaveOutcome(CombatType type, CombatResult outcome, SpellSchool school, px_t range);
    virtual void sendGotHitMessageTo(const User &user) const;
    void regen(ms_t timeElapsed);
    virtual void scaleThreatAgainst(Entity &target, double multiplier) {}
    virtual bool isAttackingTarget() const { return true; } // Assumption: entity has a target
    virtual SpellSchool school() const { return SpellSchool::PHYSICAL; }
    virtual Level level() const { return 0; }

    const Buffs &buffs() const { return _buffs; }
    const Buffs &debuffs() const { return _debuffs; }
    void applyBuff(const BuffType &type, Entity &caster);
    void applyDebuff(const BuffType &type, Entity &caster);
    void loadBuff(const BuffType &type, ms_t timeRemaining);
    void loadDebuff(const BuffType &type, ms_t timeRemaining);
    void removeDebuff(Buff::ID id);
    virtual void sendBuffMsg(const Buff::ID &buff) const;
    virtual void sendDebuffMsg(const Buff::ID &buff) const;
    void updateBuffs(ms_t timeElapsed);

    const Stats &stats() const { return _stats; }
    void stats(const Stats &stats) { _stats = stats; }
    Hitpoints health() const {return _health; }
    Energy energy() const { return _energy; }
    virtual bool canBlock() const { return false; }
    bool isStunned() const { return _stats.stunned; }

    void health(Hitpoints health) { _health = health; }
    void energy(Energy energy) { _energy = energy; }
    bool isDead() const { return _health == 0; }

    void kill() { reduceHealth(health()); }
    void reduceHealth(int damage);
    void reduceEnergy(int amount);
    void healBy(Hitpoints amount);
    virtual void onHealthChange() {}; // Probably alerting relevant users.
    virtual void onEnergyChange() {}; // Probably alerting relevant users.
    virtual void onDeath(); // Anything that needs to happen upon death.
    virtual void onAttackedBy(Entity &attacker, Threat threat); // If the entity needs to react to an attack.
    virtual void onKilled(const Entity &victim) {} // Upon this entity killing another
    virtual void sendRangedMissMessageTo(const User &userToInform) const {} // Inform user that this entity has missed its target with a ranged attack.
    virtual void sendRangedHitMessageTo(const User &userToInform) const {} // Inform user that this entity has hit its target with a ranged attack.

    virtual void describeSelfToNewWatcher(const User &watcher) const {}
    virtual void alertWatcherOnInventoryChange(const User &watcher, size_t slot) const {}
    virtual ServerItem::Slot *getSlotToTakeFromAndSendErrors(size_t slotNum, const User &user) { return nullptr; }
    virtual void onOutOfRange(const Entity &rhs) const {} // This will be called for both entities.
    virtual Message outOfRangeMessage() const { return Message(); };
    virtual bool shouldAlwaysBeKnownToUser(const User &user) const { return false; }
    
    const Loot &loot() const { assert(_loot != nullptr); return *_loot; }

    /*
    Determine whether the proposed new location is legal, considering movement speed and
    time elapsed, and checking for collisions.
    Set location to the new, legal location.
    */
    void updateLocation(const MapPoint &dest);

protected:
    void type(const EntityType *type) { _type = type; }
    std::shared_ptr<Loot> _loot;
    void resetLocationUpdateTimer() { _lastLocUpdate = SDL_GetTicks(); } // To be called when movement starts
    static const px_t MELEE_RANGE;

private:

    static size_t generateSerial();
    const EntityType *_type;
    
    Spawner *_spawner; // The Spawner that created this entity, if any.

    // Users watching this object for changes to inventory or merchant slots
    std::set<std::string> _watchers;

    // Space
    size_t _serial;
    MapPoint _location;
    ms_t _lastLocUpdate; // Time that the location was last updated; used to determine max distance


    // Combat
    Stats _stats; // Memoized stats, after gear, buffs, etc.  Calculated with updateStats();
    Hitpoints _health;
    Energy _energy;
    ms_t _attackTimer;
    Entity *_target;
    ms_t _corpseTime; // How much longer this entity should exist as a corpse.
    void startCorpseTimer();
    Buffs _buffs, _debuffs;

    ms_t _timeSinceRegen = 0;


    friend class Dummy;
};


class Dummy : public Entity{
public:
    static Dummy Serial(size_t serial) { return Dummy(serial); }
    static Dummy Location(const MapPoint &loc) { return Dummy(loc); }
    static Dummy Location(double x, double y) { return Dummy({ x, y }); }
private:
    friend class Entity;
    Dummy(size_t serial) : Entity(serial) {}
    Dummy(const MapPoint &loc) : Entity(loc) {}

    // Necessary overrides to make this a concrete class
    char classTag() const override { return 'd'; }
    void sendInfoToClient(const User &targetUser) const override {}
    ms_t timeToRemainAsCorpse() const override { return 0; }
    bool canBeAttackedBy(const User &) const override { return false; }
    static Stats _stats;
};

#endif
