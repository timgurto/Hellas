#ifndef LOOT_H
#define LOOT_H

#include "ServerItem.h"

// Describes the specific loot available when an NPC dies
class Loot{
public:
    bool empty() const;
    void add(const ServerItem *item, size_t qty);

private:
    ServerItem::vect_t _container;

};

#endif
