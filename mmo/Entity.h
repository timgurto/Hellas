#ifndef ENTITY_H
#define ENTITY_H

#include <set>

#include "EntityType.h"
#include "Point.h"

// Handles the graphical and UI side of in-game objects
class Entity{
    const EntityType &_type;
    Point _location;

public:
    Entity(const EntityType &type, const Point &location);

    bool operator<(const Entity &rhs) const; // Compares the bottom edge ("front")

    /*
    There should be no direct setter for_location, as it may invalidate a set of Entities;
    instead use one of the following:
        Client::setEntityLocation(entity, location)
        Entity::setLocation(entitiesSet, location)
        OtherUser::setLocation(entitiesSet, location)
    */
    inline const Point &location() const { return _location; }

    SDL_Rect drawRect() const;
    inline int width() const { return _type.width(); }
    inline int height() const { return _type.height(); }

    void draw() const;
    double bottomEdge() const;

    struct Compare{
        bool operator()(const Entity *lhs, const Entity *rhs) const{ return *lhs < *rhs; }
    };

    void setLocation(std::set<const Entity *, Entity::Compare> &entitiesSet, const Point &newLocation);

    typedef std::set<const Entity *, Compare> set_t;
};

#endif
