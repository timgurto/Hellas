#ifndef OBJECTS_BY_OWNER_H
#define OBJECTS_BY_OWNER_H

#include "Permissions.h"

class ObjectsByOwner{
    class ObjectsWithSpecificOwner;

public:
    const ObjectsWithSpecificOwner &getObjectsWithSpecificOwner(
            const Permissions::Owner &owner) const;

private:
    class ObjectsWithSpecificOwner{
    public:
        size_t size() const { return 1; }
    };

    std::map<Permissions::Owner, ObjectsWithSpecificOwner> container;
    static ObjectsWithSpecificOwner EmptyQueryResult;
};

#endif
