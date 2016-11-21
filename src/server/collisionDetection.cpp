// (C) 2015 Tim Gurto

#include <cassert>
#include <list>
#include <utility>

#include "CollisionChunk.h"
#include "Server.h"

const px_t Server::COLLISION_CHUNK_SIZE = 100;

bool Server::isLocationValid(const Point &loc, const ObjectType &type, const Object *thisObject){
    Rect rect = type.collisionRect() + loc;
    return isLocationValid(rect, thisObject);
}

bool Server::isLocationValid(const Rect &rect, const Object *thisObject){
    const px_t
        right = rect.x + rect.w,
        bottom = rect.y + rect.h;
    // Map edges
    const px_t
        xLimit = _mapX * Server::TILE_W - Server::TILE_W/2,
        yLimit = _mapY * Server::TILE_H;
    if (rect.x < 0 || right > xLimit ||
        rect.y < 0 || bottom > yLimit)
            return false;

    // Terrain
    size_t
        tileTop = getTileYCoord(rect.y),
        tileBottom = getTileYCoord(bottom);
    assert(tileBottom >= tileTop);
    if (tileTop == tileBottom){
        size_t
            tileLeft = getTileXCoord(rect.x, tileTop),
            tileRight = getTileXCoord(right, tileTop);
        for (size_t x = tileLeft; x <= tileRight; ++x){
            const TerrainType &terrain = _terrain[_map[x][tileTop]];
            if (!terrain.isTraversable())
                return false;
        }
    } else {
        size_t
            tileLeftEven = getTileXCoord(rect.x, 0),
            tileLeftOdd =  getTileXCoord(rect.x, 1),
            tileRightEven = getTileXCoord(right, 0),
            tileRightOdd =  getTileXCoord(right, 1);
        for (size_t y = tileTop; y <= tileBottom; ++y){
            bool yIsEven = y % 2 == 0;
            size_t
                tileLeft  = yIsEven ? tileLeftEven :  tileLeftOdd,
                tileRight = yIsEven ? tileRightEven : tileRightOdd;
            assert(tileRight >= tileLeft);
            for (size_t x = tileLeft; x <= tileRight; ++x){
                const TerrainType &terrain = _terrain[_map[x][y]];
                if (!terrain.isTraversable())
                    return false;
            }
        }
    }

    // Users
    Point rectCenter(rect.x + rect.w / 2, rect.y + rect.h / 2);
    for (const auto *user : findUsersInArea(rectCenter)) {
        if (user == thisObject)
            continue;
        if (rect.collides(user->collisionRect()))
            return false;
    }

    // Objects
    auto superChunk = getCollisionSuperChunk(rectCenter);
    for (CollisionChunk *chunk : superChunk)
        for (const auto &ret : chunk->objects()) {
            const Object *pObj = ret.second;
            if (pObj == thisObject)
                continue;
            if (!pObj->collides())
                continue;

            // Allow collisions between users and users/NPCs
            /*if (thisObject != nullptr && thisObject->classTag() == 'u' &&
                (pObj->classTag() == 'u' || pObj->classTag() == 'n'))
                    continue;*/

            if (rect.collides(pObj->collisionRect()))
                return false;
        }

    return true;
}

size_t Server::getTileYCoord(double y) const{
    size_t yTile = static_cast<size_t>(y / TILE_H);
    if (yTile >= _mapY) {
        _debug << Color::RED << "Invalid location; clipping y from " << yTile << " to " << _mapY-1
               << ". original co-ord=" << y << Log::endl;
        yTile = _mapY-1;
    }
    return yTile;
}

size_t Server::getTileXCoord(double x, size_t yTile) const{
    double originalX = x;
    if (yTile % 2 == 1)
        x += TILE_W / 2;
    size_t xTile = static_cast<size_t>(x / TILE_W);
    if (xTile >= _mapX) {
        _debug << Color::RED << "Invalid location; clipping x from " << originalX << " to "
               << _mapX-1 << ". original co-ord=" << x << Log::endl;
        xTile = _mapX-1;
    }
    return xTile;
}

std::pair<size_t, size_t> Server::getTileCoords(const Point &p) const{
    size_t
        y = getTileYCoord(p.y),
        x = getTileXCoord(p.x, y);
    return std::make_pair(x, y);
}

size_t Server::findTile(const Point &p) const{
    auto coords = getTileCoords(p);
    return _map[coords.first][coords.second];
}

CollisionChunk &Server::getCollisionChunk(const Point &p){
    size_t
        x = static_cast<size_t>(p.x / COLLISION_CHUNK_SIZE),
        y = static_cast<size_t>(p.y / COLLISION_CHUNK_SIZE);
    return _collisionGrid[x][y];
}

std::list<CollisionChunk *> Server::getCollisionSuperChunk(const Point &p) {
    size_t
        x = static_cast<size_t>(p.x / COLLISION_CHUNK_SIZE),
        minX = x - 1,
        maxX = x + 1,
        y = static_cast<size_t>(p.y / COLLISION_CHUNK_SIZE),
        minY = y - 1,
        maxY = y + 1;
    if (x == 0)
        minX = 0;
    if (y == 0)
        minY = 0;
    std::list<CollisionChunk *> superChunk;
    for (size_t x = minX; x <= maxX; ++x)
        for (size_t y = minY; y <= maxY; ++y)
            superChunk.push_back(&_collisionGrid[x][y]);
    return superChunk;
}
