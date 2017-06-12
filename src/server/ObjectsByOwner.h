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
    void add(const Permissions::Owner &owner, const Object *obj) { container[owner].add(obj); }
    void remove(const Permissions::Owner &owner, const Object *obj) { container[owner].remove(obj); }
    bool doesUserOwnObject(const std::string &username, const Object *obj) const;

private:
    class ObjectsWithSpecificOwner{
    public:
        size_t size() const { return container.size(); }
        void add(const Object *obj);
        void remove(const Object *obj);
        bool isObjectOwned(const Object *obj) const;
        
    private:
        std::set<const Object *> container;
    };

    std::map<Permissions::Owner, ObjectsWithSpecificOwner> container;
    static ObjectsWithSpecificOwner EmptyQueryResult;
};

#endif
