#include <cassert>

#include "ObjectsByOwner.h"

ObjectsByOwner::ObjectsWithSpecificOwner ObjectsByOwner::EmptyQueryResult;

const ObjectsByOwner::ObjectsWithSpecificOwner &ObjectsByOwner::getObjectsWithSpecificOwner(
            const Permissions::Owner &owner) const{
    auto it = container.find(owner);
    if (it == container.end())
        return EmptyQueryResult;
    return it->second;
}

bool ObjectsByOwner::doesUserOwnObject(const std::string &username, const Object *obj) const{
    Permissions::Owner owner(Permissions::Owner::PLAYER, username);
    const auto &hisObjects = getObjectsWithSpecificOwner(owner);
    return hisObjects.isObjectOwned(obj);
}

void ObjectsByOwner::ObjectsWithSpecificOwner::add(const Object *obj){
    container.insert(obj);
}

void ObjectsByOwner::ObjectsWithSpecificOwner::remove(const Object *obj){
    container.erase(obj);
}

bool ObjectsByOwner::ObjectsWithSpecificOwner::isObjectOwned(const Object *obj) const{
    auto it = container.find(obj);
    return it != container.end();
}
