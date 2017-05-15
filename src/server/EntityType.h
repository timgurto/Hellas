#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

#include <string>

#include "TerrainList.h"
#include "../Rect.h"

class EntityType{
public:
    EntityType(const std::string id);
    const std::string &id() const { return _id; }
    bool isTag( const std::string &tagName) const;
    void addTag(const std::string &tagName);
    virtual char classTag() const = 0;

    // Space
    bool collides() const { return _collides; }
    const Rect &collisionRect() const { return _collisionRect; }
    void collisionRect(const Rect &r) { _collisionRect = r; _collides = true; }
    const TerrainList &allowedTerrain() const;
    void allowedTerrain(const std::string &id){ _allowedTerrain = TerrainList::findList(id); }


    // Combat

private:
    std::string _id;
    std::set<std::string> _tags;

    bool operator<(const EntityType &rhs) const { return _id < rhs._id; }


    // Space
    bool _collides; // false by default; true if any collisionRect is specified.
    Rect _collisionRect; // Relative to position

    const TerrainList *_allowedTerrain;


    // Combat
};

#endif
