// (C) 2015 Tim Gurto

#ifndef BRANCH_H
#define BRANCH_H

#include <sstream>
#include <string>

#include "Entity.h"
#include "Point.h"

// Describes a tree branch which can be collected by a user
class Branch : public Entity{
    static EntityType _entityType;

    size_t _serial;

public:

    Branch(const Branch &rhs);
    // No location: create dummy Branch, for set searches
    Branch(size_t serial, const Point &loc = 0);

    bool operator<(const Branch &rhs) const { return _serial < rhs._serial; }
    bool operator==(const Branch &rhs) const { return _serial == rhs._serial; }

    static void image(const std::string &filename) { _entityType.image(filename); }
    size_t serial() const { return _serial; }

    virtual void onLeftClick(Client &client) const;
    virtual std::vector<std::string> getTooltipMessages(const Client &client) const;
};

#endif
