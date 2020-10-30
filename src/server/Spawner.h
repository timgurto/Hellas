#ifndef SPAWNER_H
#define SPAWNER_H

#include <list>
#include <set>

#include "../Point.h"
#include "../util.h"

class Server;
class ObjectType;
class Entity;
class TerrainList;

class Spawner {
  MapPoint _location;
  double _radius;           // Default: 0
  const ObjectType *_type;  // What it spawns
  size_t _quantity;         // How many to maintain.  Default: 1
  bool _shouldUseTerrainCache{false};

  // Time between an object being removed, and its replacement spawning.
  // Default: 0 In the case of an NPC, this timer starts on death, rather than
  // on the corpse despawning.
  ms_t _respawnTime;

  std::set<char> _terrainWhitelist;  // Only applies if nonempty
  std::list<ms_t>
      _spawnSchedule;  // The times at which new objects should spawn

  class TerrainCache {
    std::vector<size_t> _validTiles1D;
    const Spawner &_owner;

   public:
    TerrainCache(const Spawner &owner);
    void registerValidTile(size_t x, size_t y);
    std::pair<size_t, size_t> pickRandomTile() const;
    void cacheTiles();
  };
  TerrainCache _terrainCache;

 public:
  Spawner(const MapPoint &location = MapPoint{},
          const ObjectType *type = nullptr);
  void initialise();

  MapPoint getRandomPoint() const;
  const ObjectType *type() const { return _type; }
  void radius(double r) { _radius = r; }
  void quantity(size_t qty) { _quantity = qty; }
  size_t quantity() const { return _quantity; }
  void respawnTime(ms_t t) { _respawnTime = t; }
  void allowTerrain(char c) { _terrainWhitelist.insert(c); }
  void useTerrainCache() { _shouldUseTerrainCache = true; }

  const Entity *spawn();  // Attempt to add a new object.
  // Add a spawn job to the queue.  After _respawnTime, spawn() will be called.
  void scheduleSpawn();
  void update(ms_t currentTime);  // Act on any scheduled spawns that are due.
};

#endif
