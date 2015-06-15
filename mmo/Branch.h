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

    bool operator<(const Branch &rhs) const; // Compare serials
    bool operator==(const Branch &rhs) const;

    friend std::ostream &operator<<(std::ostream &lhs, const Branch &rhs);

    static void image(const std::string &filename);
    int serial() const;
    const Point &location() const;
    const Entity &entity() const;
};

#endif
