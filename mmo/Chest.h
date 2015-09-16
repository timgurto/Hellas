// (C) 2015 Tim Gurto

#ifndef CHEST_H
#define CHEST_J

#include <sstream>
#include <string>

#include "Entity.h"
#include "Point.h"

// Describes a chest, which stores items.
class Chest : public Entity{
    static EntityType _entityType;

    size_t _serial;

public:

    Chest(const Chest &rhs);
    // No location: create dummy Chest, for set searches
    Chest(size_t serial, const Point &loc = 0);

    bool operator<(const Chest &rhs) const { return _serial < rhs._serial; }
    bool operator==(const Chest &rhs) const { return _serial == rhs._serial; }

    static void image(const std::string &filename) { _entityType.image(filename); }
    size_t serial() const { return _serial; }

    virtual void onLeftClick(Client &client) const;
    virtual std::vector<std::string> getTooltipMessages(const Client &client) const;
};

#endif
