// (C) 2015 Tim Gurto

#ifndef ITEM_H
#define ITEM_H

#include <map>
#include <set>
#include <string>

#include "Texture.h"

// Describes an item type
class Item{
    std::string _id; // The no-space, unique name used in data files
    std::string _name;
    size_t _stackSize;
    std::set<std::string> _classes;
    Texture _icon;

    /*
    The materials necessary to craft this item.
    An empty map implies an item that cannot be crafted.
    */
    std::map<std::string, size_t> _materials;
    Uint32 _craftTime; // How long this item takes to craft

public:
    Item(const std::string &id, const std::string &name, size_t stackSize = 1);
    Item(const std::string &id); // Creates a dummy Item for set lookup

    bool operator<(const Item &rhs) const { return _id < rhs._id; }

    const std::string &id() const { return _id; }
    const std::string &name() const { return _name; }
    size_t stackSize() const { return _stackSize; }
    const Texture &icon() const { return _icon; }
    const std::set<std::string> &classes() const { return _classes; }
    const std::map<std::string, size_t> &materials() const { return _materials; }
    Uint32 craftTime() const { return _craftTime; }
    void craftTime(Uint32 time) { _craftTime = time * 1000; }

    void addClass(const std::string &className);
    bool hasClasses() const { return _classes.size() > 0; }
    bool isClass(const std::string &className) const;

    void addMaterial(const std::string &id, size_t quantity = 1);
    bool isCraftable() const { return !_materials.empty(); }
};

#endif
