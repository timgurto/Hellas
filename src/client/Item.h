// (C) 2015 Tim Gurto

#ifndef ITEM_H
#define ITEM_H

#include <map>
#include <string>
#include <set>
#include <vector>

#include "Texture.h"

// The client-side representation of an item type
class Item{
    std::string _id;
    std::string _name;
    Texture _icon;

    // Used by the crafting window
    std::set<std::string> _classes;
    std::map<const Item *, size_t> _materials;

public:
    Item(const std::string &id, const std::string &name = "");

    const std::string &name() const { return _name; }
    const Texture &icon() const { return _icon; }

    typedef std::vector<std::pair<const Item *, size_t> > vect_t;
    
    bool operator<(const Item &rhs) const { return _id < rhs._id; }

    const std::string &id() const { return _id; }
    //size_t stackSize() const { return _stackSize; }
    //void stackSize(size_t n) { _stackSize = n; }
    void icon(const std::string &filename);
    const std::set<std::string> &classes() const { return _classes; }
    const std::map<const Item *, size_t> &materials() const { return _materials; }
    //Uint32 craftTime() const { return _craftTime; }
    //void craftTime(Uint32 time) { _craftTime = time; }
    //void constructsObject(const ObjectType *obj) { _constructsObject = obj; }
    //const ObjectType *constructsObject() const { return _constructsObject; }

    void addClass(const std::string &className);
    bool hasClasses() const { return _classes.size() > 0; }
    bool isClass(const std::string &className) const;

    void addMaterial(const Item *id, size_t quantity = 1);
    bool isCraftable() const { return !_materials.empty(); }
};

#endif
