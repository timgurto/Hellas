// (C) 2015 Tim Gurto

#include <list>
#include <utility>

#include "CollisionChunk.h"
#include "Server.h"

const int Server::COLLISION_CHUNK_SIZE = 100;

bool Server::isLocationValid(const Point &loc, const ObjectType &type,
                             const Object *thisObject, const User *thisUser){
    Rect rect = type.collisionRect() + loc;
    const int
        right = rect.x + rect.w,
        bottom = rect.y + rect.h;
    // Map edges
    const int
        xLimit = _mapX * Server::TILE_W - Server::TILE_W/2,
        yLimit = _mapY * Server::TILE_H;
    if (rect.x < 0 || right > xLimit ||
        rect.y < 0 || bottom > yLimit)
        return false;

    // Terrain
    CollisionChunk chunk = getCollisionChunk(loc);
    auto coords = getTileCoords(loc);
    const Terrain &terrain = *_terrain.find(_map[coords.first][coords.second]);
    if (!terrain.isTraversable())
        return false;

    // Users
    for (const auto &user : _users) {
        if (&user == thisUser)
            continue;
        if (rect.collides(user.location() + User::OBJECT_TYPE.collisionRect()))
            return false;
    }

    // Objects
    auto superChunk = getCollisionSuperChunk(loc);
    for (CollisionChunk *chunk : superChunk)
        for (const auto &ret : chunk->objects()) {
            const Object *pObj = ret.second;
            if (rect.collides(pObj->collisionRect()))
                return false;
        }

    return true;
}

std::pair<size_t, size_t> Server::getTileCoords(const Point &p) const{
    size_t y = static_cast<size_t>(p.y / TILE_H);
    if (y >= _mapY) {
        _debug << Color::RED << "Invalid location; clipping y from " << y << " to " << _mapY-1
               << ". original co-ord=" << p.y << Log::endl;
        y = _mapY-1;
    }
    double rawX = p.x;
    if (y % 2 == 1)
        rawX += TILE_W/2;
    size_t x = static_cast<size_t>(rawX / TILE_W);
    if (x >= _mapX) {
        _debug << Color::RED << "Invalid location; clipping x from " << x << " to " << _mapX-1
               << ". original co-ord=" << p.x << Log::endl;
        x = _mapX-1;
    }
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
