#ifndef BRANCH_H
#define BRANCH_H

#include <sstream>
#include <string>

#include "Entity.h"
#include "Point.h"

// Describes a tree branch which can be collected by a user
class Branch{
    static int _currentSerial;
    static EntityType _entityType;

    int _serial;
    Entity _entity;

public:

    Branch(const Branch &rhs);
    Branch(const Point &loc); // Generates new serial; should only be called by server
    Branch(int serial, const Point &loc = 0); // No location: create dummy Branch, for set searches

    inline bool operator<(const Branch &rhs) const { return _serial < rhs._serial; }
    inline bool operator==(const Branch &rhs) const { return _serial == rhs._serial; }

    inline static void image(const std::string &filename) { _entityType.image(filename); }
    inline int serial() const { return _serial; }
    inline const Point &location() const { return _entity.location(); }
    inline const Entity &entity() const { return _entity; }
};

#endif
