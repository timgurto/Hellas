#include <list>
#include <utility>

#include "CollisionChunk.h"
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

MapRect Server::getTileRect(size_t x, size_t y) {
  MapRect r(static_cast<px_t>(x * TILE_W), static_cast<px_t>(y * TILE_H),
            TILE_W, TILE_H);
  if (y % 2 == 0) r.x -= TILE_W / 2;
  return r;
}

std::set<char> Server::nearbyTerrainTypes(const MapRect &rect,
                                          double extraRadius) {
  if (extraRadius < 0) {
    SERVER_ERROR("Terrain-lookup radius is negative.  Setting to 0.");
    extraRadius = 0;
  }
  std::set<char> tilesInRect;
  const double left = max(0, rect.x - extraRadius),
               right = rect.x + rect.w + extraRadius,
               top = max(0, rect.y - extraRadius),
               bottom = rect.y + rect.h + extraRadius;
  size_t tileTop = getTileYCoord(top), tileBottom = getTileYCoord(bottom);
  if (tileBottom < tileTop) {
    SERVER_ERROR("Can't look up terrain: bottom index is less than top index.");
    return tilesInRect;
  }

  // Single row
  if (tileTop == tileBottom) {
    size_t tileLeft = getTileXCoord(rect.x, tileTop),
           tileRight = getTileXCoord(right, tileTop);
    for (size_t x = tileLeft; x <= tileRight; ++x)
      tilesInRect.insert(_map[x][tileTop]);

    // General case
  } else {
    size_t tileLeftEven = getTileXCoord(left, 0),
           tileLeftOdd = getTileXCoord(left, 1),
           tileRightEven = getTileXCoord(right, 0),
           tileRightOdd = getTileXCoord(right, 1);
    for (size_t y = tileTop; y <= tileBottom; ++y) {
      bool yIsEven = y % 2 == 0;
      size_t tileLeft = yIsEven ? tileLeftEven : tileLeftOdd,
             tileRight = yIsEven ? tileRightEven : tileRightOdd;
      if (tileRight < tileLeft) {
        SERVER_ERROR(
            "Can't look up terrain: right index is less than left index.");
        return tilesInRect;
      }
      for (size_t x = tileLeft; x <= tileRight; ++x) {
        char terrainIndex = _map[x][y];
        // Exclude if outside radius
        if (extraRadius != 0) {
          if (tilesInRect.find(terrainIndex) != tilesInRect.end()) continue;
          if (distance(rect, getTileRect(x, y)) > extraRadius) continue;
        }
        tilesInRect.insert(terrainIndex);
      }
    }
  }
  return tilesInRect;
}

bool Server::isLocationValid(const MapRect &rect,
                             const TerrainList &allowedTerrain,
                             const Entity *thisEntity) {
  // A user in a vehicle is unrestricted; the vehicle's restrictions will
  // dictate his location.
  if (thisEntity != nullptr && thisEntity->classTag() == 'u' &&
      dynamic_cast<const User *>(thisEntity)->isDriving())
    return true;

  const double right = rect.x + rect.w, bottom = rect.y + rect.h;
  // Map edges
  const double xLimit = _mapX * Server::TILE_W - Server::TILE_W / 2,
               yLimit = _mapY * Server::TILE_H;
  if (rect.x < 0 || right > xLimit || rect.y < 0 || bottom > yLimit) {
    return false;
  }

  // Terrain
  auto terrainTypesCovered = nearbyTerrainTypes(rect);
  for (char terrainType : terrainTypesCovered)
    if (!allowedTerrain.allows(terrainType)) return false;

  // Objects
  MapPoint rectCenter(rect.x + rect.w / 2, rect.y + rect.h / 2);
  auto superChunk = getCollisionSuperChunk(rectCenter);
  for (CollisionChunk *chunk : superChunk)
    for (const auto &pair : chunk->entities()) {
      const Entity *pEnt = pair.second;
      if (pEnt == thisEntity) continue;
      if (!pEnt->collides()) continue;

      // Allow collisions between users and users/NPCs
      /*if (thisObject != nullptr && thisObject->classTag() == 'u' &&
          (pObj->classTag() == 'u' || pObj->classTag() == 'n'))
              continue;*/

      if ((pEnt->classTag() == 'u') &&
          dynamic_cast<const User *>(pEnt)->isDriving())
        continue;

      if (rect.collides(pEnt->collisionRect())) return false;
    }

  return true;
}

size_t Server::getTileYCoord(double y) const {
  if (y < 0) {
    SERVER_ERROR("Attempting to get tile for negative y co-ord");
    return 0;
  }
  size_t yTile = static_cast<size_t>(y / TILE_H);
  if (yTile >= _mapY) {
    _debug << Color::CHAT_ERROR << "Invalid location; clipping y from " << yTile
           << " to " << _mapY - 1 << ". original co-ord=" << y << Log::endl;
    yTile = _mapY - 1;
  }
  return yTile;
}

size_t Server::getTileXCoord(double x, size_t yTile) const {
  if (x < 0) {
    SERVER_ERROR("Attempting to get tile for negative x co-ord");
    return 0;
  }
  double originalX = x;
  if (yTile % 2 == 1) x += TILE_W / 2;
  size_t xTile = static_cast<size_t>(x / TILE_W);
  if (xTile >= _mapX) {
    _debug << Color::CHAT_ERROR << "Invalid location; clipping x from "
           << originalX << " to " << _mapX - 1 << ". original co-ord=" << x
           << Log::endl;
    xTile = _mapX - 1;
  }
  return xTile;
}

std::pair<size_t, size_t> Server::getTileCoords(const MapPoint &p) const {
  size_t y = getTileYCoord(p.y), x = getTileXCoord(p.x, y);
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

std::list<CollisionChunk *> Server::getCollisionSuperChunk(const MapPoint &p) {
  size_t x = static_cast<size_t>(p.x / COLLISION_CHUNK_SIZE), minX = x - 1,
         maxX = x + 1, y = static_cast<size_t>(p.y / COLLISION_CHUNK_SIZE),
         minY = y - 1, maxY = y + 1;
  if (x == 0) minX = 0;
  if (y == 0) minY = 0;
  std::list<CollisionChunk *> superChunk;
  for (size_t x = minX; x <= maxX; ++x)
    for (size_t y = minY; y <= maxY; ++y)
      superChunk.push_back(&_collisionGrid[x][y]);
  return superChunk;
}
