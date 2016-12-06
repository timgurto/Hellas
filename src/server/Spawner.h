#ifndef SPAWNER_H
#define SPAWNER_H

#include <queue>

#include "../Point.h"
#include "../util.h"

class ObjectType;

class Spawner{
    Point _location;
    double _range; // Default: 0
    const ObjectType *_type; // What it spawns
    size_t _quantity; // How many to maintain.  Default: 1

    //Time between an object being removed, and its replacement spawning.  Default: 0
    ms_t _respawnTime;

    std::vector<size_t> _terrainWhitelist; // Only applies if nonempty
    std::queue<ms_t> _respawnQueue; // The times at which new objects should spawn

public:
    Spawner(const Point &location, const ObjectType *type);

    void range(double r) { _range = r; }
    void quantity(size_t qty) { _quantity = qty; }
    void respawnTime(ms_t t) { _respawnTime = t; }
    void allowTerrain(size_t n) { _terrainWhitelist.push_back(n); }
};

#endif
