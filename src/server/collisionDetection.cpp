#include <list>
#include <utility>

#include "CollisionChunk.h"
#include "DroppedItem.h"
#include "Entity.h"
#include "Server.h"

const px_t Server::COLLISION_CHUNK_SIZE = 160;

bool Server::isLocationValid(const MapPoint &loc, const EntityType &type) {
  auto rect = type.collisionRect() + loc;
  return isLocationValid(rect, type.allowedTerrain());
}

bool Server::isLocationValid(const MapPoint &loc, const Entity &thisEntity) {
  auto rect = thisEntity.type()->collisionRect() + loc;
  return isLocationValid(rect, thisEntity);
}

bool Server::isLocationValid(const MapRect &rect, const Entity &thisEntity) {
  return isLocationValid(rect, thisEntity.allowedTerrain(), &thisEntity);
}

bool Server::isLocationValid(const MapRect &rect,
                             const TerrainList &allowedTerrain,
                             const Entity *thisEntity) {
  // A user in a vehicle is unrestricted; the vehicle's restrictions will
  // dictate his location.
  if (thisEntity && !thisEntity->collides()) return true;

  const double right = rect.x + rect.w, bottom = rect.y + rect.h;
  // Map edges
  const double xLimit = _map.width() * Map::TILE_W - Map::TILE_W / 2,
               yLimit = _map.height() * Map::TILE_H;
  if (rect.x < 0 || right > xLimit || rect.y < 0 || bottom > yLimit) {
    return false;
  }

  // Terrain
  auto terrainTypesCovered = _map.terrainTypesOverlapping(rect);
  for (char terrainType : terrainTypesCovered)
    if (!allowedTerrain.allows(terrainType)) return false;

  // Objects
  auto superChunk = getAllCollisionChunksTouchingRect(rect);
  for (const auto *chunk : superChunk)
    for (const auto &pair : chunk->entities()) {
      const Entity *pEnt = pair.second;
      if (pEnt == thisEntity) continue;
      if (!pEnt->collides()) continue;

      if (thisEntity && pEnt->areOverlapsAllowedWith(*thisEntity)) continue;

      if (rect.overlaps(pEnt->collisionRect())) return false;
    }

  return true;
}

std::pair<size_t, size_t> Server::getTileCoords(const MapPoint &p) const {
  size_t y = _map.getRow(p.y), x = _map.getCol(p.x, y);
  return std::make_pair(x, y);
}

char Server::findTile(const MapPoint &p) const {
  auto coords = getTileCoords(p);
  return _map[coords.first][coords.second];
}

CollisionChunk &Server::getCollisionChunk(const MapPoint &p) {
  size_t x = static_cast<size_t>(p.x / COLLISION_CHUNK_SIZE),
         y = static_cast<size_t>(p.y / COLLISION_CHUNK_SIZE);
  return _collisionGrid[x][y];
}

std::list<const CollisionChunk *> Server::getAllCollisionChunksTouchingRect(
    const MapRect &r) {
  auto chunks = std::list<const CollisionChunk *>{};
  auto left = static_cast<size_t>(r.x / COLLISION_CHUNK_SIZE),
       right = static_cast<size_t>((r.x + r.w) / COLLISION_CHUNK_SIZE),
       top = static_cast<size_t>(r.y / COLLISION_CHUNK_SIZE),
       bottom = static_cast<size_t>((r.y + r.h) / COLLISION_CHUNK_SIZE);

  // Consider neighbours, in case an object covers two chunks
  if (left > 0) --left;
  if (top > 0) --top;
  ++right;
  ++bottom;

  for (auto x = left; x <= right; ++x) {
    const auto rowIt = _collisionGrid.find(x);
    if (rowIt == _collisionGrid.end()) continue;
    const auto &row = rowIt->second;

    for (auto y = top; y <= bottom; ++y) {
      const auto chunkIter = row.find(y);
      if (chunkIter == row.end()) continue;
      const auto &chunk = chunkIter->second;

      chunks.push_back(&chunk);
    }
  }

  return chunks;
}

// For the functions below:
//          USER  NPC    GATE  ITEM OTHER   (this)
//   USER    T     T      ?     T
//   NPC     T            ?*    T
//   GATE    ?     ?*           T
//   ITEM    T     T      T     T
//   OTHER
//   (rhs)
// ?: Depends on permissions
// Blank: false

bool Entity::areOverlapsAllowedWith(const Entity &rhs) const { return false; }

bool User::areOverlapsAllowedWith(const Entity &rhs) const {
  if (rhs.classTag() == 'u' || rhs.classTag() == 'n') return true;

  if (rhs.classTag() == 'o') {
    const auto &obj = dynamic_cast<const Object &>(rhs);
    if (obj.isGate()) return obj.permissions.canUserOverlap(_name);
  }
  return false;
}

bool NPC::areOverlapsAllowedWith(const Entity &rhs) const {
  if (rhs.classTag() == 'u') return true;

  if (rhs.classTag() == 'o') {
    const auto &obj = dynamic_cast<const Object &>(rhs);
    if (obj.isGate()) return obj.permissions.canNPCOverlap(*this);
  }

  return false;
}

bool Object::areOverlapsAllowedWith(const Entity &rhs) const {
  if (!isGate()) return false;
  if (rhs.classTag() == 'u') {
    const auto &user = dynamic_cast<const User &>(rhs);
    return permissions.canUserOverlap(user.name());
  }
  if (rhs.classTag() == 'n') {
    const auto &npc = dynamic_cast<const NPC &>(rhs);
    return permissions.canNPCOverlap(npc);
  }
  return false;
}

bool DroppedItem::areOverlapsAllowedWith(const Entity &rhs) const {
  if (rhs.classTag() == 'o') return false;
  return true;
}
