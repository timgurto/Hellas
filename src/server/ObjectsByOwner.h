#ifndef OBJECTS_BY_OWNER_H
#define OBJECTS_BY_OWNER_H

#include <set>

#include "Permissions.h"

class Object;

class ObjectsByOwner{
    class ObjectsWithSpecificOwner;

public:
    bool isObjectOwnedBy(size_t serial, const Permissions::Owner &owner) const;
    const ObjectsWithSpecificOwner &getObjectsWithSpecificOwner(
            const Permissions::Owner &owner) const;
    void add(const Permissions::Owner &owner, size_t serial) { container[owner].add(serial); }
    void remove(const Permissions::Owner &owner, size_t serial) { container[owner].remove(serial); }

private:
    class ObjectsWithSpecificOwner{
    public:
        size_t size() const { return container.size(); }
        void add(size_t serial);
        void remove(size_t serial);
        bool isObjectOwned(size_t serial) const;
        
    private:
        std::set<size_t> container;
    };

    std::map<Permissions::Owner, ObjectsWithSpecificOwner> container;
    static ObjectsWithSpecificOwner EmptyQueryResult;
};

#endif
