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