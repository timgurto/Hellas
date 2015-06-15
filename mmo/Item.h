#ifndef ITEM_H
#define ITEM_H

#include <string>

#include "Texture.h"

// Describes an item type
class Item{
    std::string _id; // The no-space, unique name used in data files
    std::string _name;
    size_t _stackSize;
    Texture _icon;

public:
    Item(const std::string &id, const std::string &name, size_t stackSize = 1);
    Item(const std::string &id); // Creates a dummy Item for set lookup

    bool operator<(const Item &rhs) const; // Compares ids

    const std::string &id() const;
    size_t stackSize() const;
    const Texture &icon();
};

#endif
