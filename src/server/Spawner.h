#ifndef SPAWNER_H
#define SPAWNER_H

#include <list>
#include <set>

#include "../Point.h"
#include "../util.h"

class Server;
class ObjectType;

class Spawner{
    size_t _index;
    Point _location;
    double _radius; // Default: 0
    const ObjectType *_type; // What it spawns
    size_t _quantity; // How many to maintain.  Default: 1

    // Time between an object being removed, and its replacement spawning.  Default: 0
    // In the case of an NPC, this timer starts on death, rather than on the corpse despawning.
    ms_t _respawnTime;

    std::set<size_t> _terrainWhitelist; // Only applies if nonempty
    std::list<ms_t> _spawnSchedule; // The times at which new objects should spawn

public:
    Spawner(size_t index = 0, const Point &location = Point(), const ObjectType *type = nullptr);

    size_t index() const { return _index; }
    const ObjectType *type() const { return _type; }
    void radius(double r) { _radius = r; }
    void quantity(size_t qty) { _quantity = qty; }
    size_t quantity() const { return _quantity; }
    void respawnTime(ms_t t) { _respawnTime = t; }
    void allowTerrain(size_t n) { _terrainWhitelist.insert(n); }

    void spawn(); // Attempt to add a new object.
    // Add a spawn job to the queue.  After _respawnTime, spawn() will be called.
    void scheduleSpawn();
    void update(ms_t currentTime); // Act on any scheduled spawns that are due.
};

#endif
