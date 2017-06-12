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

void ObjectsByOwner::ObjectsWithSpecificOwner::add(const Object *obj){
    container.insert(obj);
}

void ObjectsByOwner::ObjectsWithSpecificOwner::remove(const Object *obj){
    container.erase(obj);
}