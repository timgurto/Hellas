#include "Spawner.h"

#include "Server.h"

Spawner::TerrainCache Spawner::terrainCache;

Spawner::Spawner(const MapPoint &location, const ObjectType *type)
    : _location(location),
      _type(type),

      _radius(0),
      _quantity(1),
      _respawnTime(0) {}

MapPoint Spawner::getRandomPoint() const {
  MapPoint p = _location;
  if (_radius != 0) {
    double radius = sqrt(randDouble()) * _radius;
    double angle = randDouble() * 2 * PI;
    p.x += cos(angle) * radius;
    p.y -= sin(angle) * radius;
  }
  return p;
}

double Spawner::distanceFromEntity(const Entity &entity) const {
  return distance(_location, entity.location());
}

void Spawner::spawn() {
  static const size_t MAX_ATTEMPTS = 50;
  Server &server = *Server::_instance;

  for (size_t attempt = 0; attempt != MAX_ATTEMPTS; ++attempt) {
    auto p = MapPoint{};
    if (_shouldUseTerrainCache) {
      auto tile = terrainCache.pickRandomTile(_type->allowedTerrain());
      p = Map::randomPointInTile(tile.first, tile.second);
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
    return;
  }

  server._debug << Color::CHAT_ERROR << "Failed to spawn " << _type->id()
                << Log::endl;
  scheduleSpawn();
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

void Spawner::initialise() {
  cacheTerrain("water");
  cacheTerrain("default");
}

void Spawner::cacheTerrain(std::string listID) {
  auto &server = Server::instance();
  auto terrainList = TerrainList::findList(listID);
  if (!terrainList) return;
  for (auto x = 0; x != server.map().width(); ++x)
    for (auto y = 0; y != server.map().height(); ++y) {
      auto terrainAtThisTile = server.map()[x][y];
      if (terrainList->allows(terrainAtThisTile))
        terrainCache.registerValidTile(*terrainList, x, y);
    }
}

void Spawner::TerrainCache::registerValidTile(const TerrainList &terrainList,
                                              size_t x, size_t y) {
  auto &server = Server::instance();
  auto tile = server.map().to1D(x, y);
  _validTiles[&terrainList].push_back(tile);
}

std::pair<size_t, size_t> Spawner::TerrainCache::pickRandomTile(
    const TerrainList &terrainList) const {
  auto it = _validTiles.find(&terrainList);
  auto pair = it->second;
  auto tile = it->second.front();
  return Server::instance().map().from1D(tile);
}
