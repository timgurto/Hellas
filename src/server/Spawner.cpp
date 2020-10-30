#include "Spawner.h"

#include "Server.h"

Spawner::Spawner(const MapPoint &location, const ObjectType *type)
    : _location(location),
      _type(type),

      _radius(0),
      _quantity(1),
      _respawnTime(0),
      _terrainCache(*this) {}

void Spawner::initialise() {
  if (_shouldUseTerrainCache) _terrainCache.cacheTiles();
}

MapPoint Spawner::getRandomPoint() const {
  return getRandomPointInCircle(_location, _radius);
}

const Entity *Spawner::spawn() {
  static const size_t MAX_ATTEMPTS = 50;
  Server &server = *Server::_instance;

  for (size_t attempt = 0; attempt != MAX_ATTEMPTS; ++attempt) {
    auto p = MapPoint{};
    if (_shouldUseTerrainCache) {
      auto tile = _terrainCache.pickRandomTile();
      p = Map::randomPointInTile(tile.first, tile.second);

      if (distance(p, _location) > _radius) continue;
    } else
      p = getRandomPoint();

    // Check terrain whitelist
    if (!_terrainWhitelist.empty()) {
      char terrain = server.findTile(p);
      if (_terrainWhitelist.find(terrain) == _terrainWhitelist.end()) continue;
    }

    // Check location validity
    if (!server.isLocationValid(p, *_type)) continue;

    // Add object;
    Entity *entity;
    if (_type->classTag() == 'n')
      entity = &server.addNPC(dynamic_cast<const NPCType *>(_type), p);
    else
      entity =
          &server.addObject(dynamic_cast<const ObjectType *>(_type), p, {});
    entity->spawner(this);
    entity->excludeFromPersistentState();
    return entity;
  }

  server._debug << Color::CHAT_ERROR << "Failed to spawn " << _type->id()
                << Log::endl;
  scheduleSpawn();
  return nullptr;
}

void Spawner::scheduleSpawn() {
  Log &d = Server::_instance->_debug;
  _spawnSchedule.push_back(SDL_GetTicks() + _respawnTime);
}

void Spawner::update(ms_t currentTime) {
  while (!_spawnSchedule.empty() && _spawnSchedule.front() <= currentTime) {
    _spawnSchedule.pop_front();
    spawn();
  }
}

void Spawner::TerrainCache::cacheTiles() {
  const auto &terrainList = _owner.type()->allowedTerrain();

  auto &server = Server::instance();
  for (auto x = 0; x != server.map().width(); ++x)
    for (auto y = 0; y != server.map().height(); ++y) {
      // Check terrain is in list
      auto terrainAtThisTile = server.map()[x][y];
      if (!terrainList.allows(terrainAtThisTile)) continue;

      // Check that it's inside the spawn point's radius
      auto tileRect = server.map().getTileRect(x, y);
      if (distance(tileRect, MapRect{_owner._location}) > _owner._radius)
        continue;

      registerValidTile(x, y);
    }
}

Spawner::TerrainCache::TerrainCache(const Spawner &owner) : _owner(owner) {}

void Spawner::TerrainCache::registerValidTile(size_t x, size_t y) {
  auto &server = Server::instance();
  auto tile = server.map().to1D(x, y);
  _validTiles1D.push_back(tile);
}

std::pair<size_t, size_t> Spawner::TerrainCache::pickRandomTile() const {
  if (_validTiles1D.empty()) {
    SERVER_ERROR("No valid tiles for cached spawner");
    return {0, 0};
  }
  auto randomIndex = rand() % _validTiles1D.size();
  auto tile = _validTiles1D[randomIndex];
  return Server::instance().map().from1D(tile);
}
