// (C) 2015 Tim Gurto

#ifndef TREE_H
#define TREE_H

#include <sstream>
#include <string>

#include "Entity.h"
#include "Point.h"

// Describes a tree which can be chopped down by a user.
class Tree : public Entity{
    static EntityType _entityType;

    size_t _serial;

public:

    Tree(const Tree &rhs);
    Tree(size_t serial, const Point &loc = 0); // No location: create dummy Branch, for set searches

    bool operator<(const Tree &rhs) const { return _serial < rhs._serial; }
    bool operator==(const Tree &rhs) const { return _serial == rhs._serial; }

    static void image(const std::string &filename) { _entityType.image(filename); }
    size_t serial() const { return _serial; }

    virtual void onLeftClick(Client &client) const;
    virtual std::vector<std::string> getTooltipMessages(const Client &client) const;
};

#endif
