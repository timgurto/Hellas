// (C) 2015 Tim Gurto

#include "Server.h"

bool Server::isLocationValid(const Point &loc, const ObjectType &type,
                             const Object *thisObject, const User *thisUser) const{
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
    size_t terrain = findTile(loc);
    if (terrain == 3 || terrain == 4)
        return false;

    // Users
    for (const auto &user : _users) {
        if (&user == thisUser)
            continue;
        if (rect.collides(user.location() + User::OBJECT_TYPE.collisionRect()))
            return false;
    }

    // Objects
    for (const Object obj : _objects) {
        if (&obj == thisObject)
            continue;
        if (!obj.type()->collides())
            continue;
        if (rect.collides(obj.collisionRect()))
            return false;
    }
    return true;
}
