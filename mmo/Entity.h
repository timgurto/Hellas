#ifndef ENTITY_H
#define ENTITY_H

#include "EntityType.h"
#include "Point.h"

// Handles the graphical and UI side of in-game objects
class Entity{
    const EntityType &_type;
    Point _location;

public:
    Entity(const EntityType &type, const Point &location);

    bool operator<(const Entity &rhs) const; // Compares the bottom edge ("front")

    const Point &location() const;
    void locationInner(const Point &location); // Shouldn't usually be called directly; instead call Client::setClientLocation().
    SDL_Rect drawRect() const;
    int width() const;
    int height() const;

    void draw() const;
    double bottomEdge() const;

};

#endif
