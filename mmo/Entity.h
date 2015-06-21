#ifndef ENTITY_H
#define ENTITY_H

#include <set>

#include "EntityType.h"
#include "Point.h"
#include "Texture.h"

class Client;

// Handles the graphical and UI side of in-game objects  Abstract class
class Entity{
    bool _yChanged; // y co-ordinate has changed, and the entity must be reordered.

protected:
    const EntityType &_type;
    Point _location;

public:
    Entity(const EntityType &type, const Point &location);

    inline const Point &location() const { return _location; }
    void location(const Point &loc); // yChanged() should be checked after changing location.
    SDL_Rect drawRect() const;
    inline int width() const { return _type.width(); }
    inline int height() const { return _type.height(); }
    inline bool yChanged() const { return _yChanged; }
    inline void yChanged(bool val) { _yChanged = val; }

    virtual void draw(const Client &client) const;
    virtual void update(double delta) {}
    virtual void onLeftClick(const Client &client) const {}
    virtual Texture tooltip(const Client &client) const { return Texture(); }

    double bottomEdge() const;
    bool collision(const Point &p) const;

    struct ComparePointers{
        bool operator()(const Entity *lhs, const Entity *rhs) const{

            // 1. location
            double
                lhsBottom = lhs->bottomEdge(),
                rhsBottom = rhs->bottomEdge();
            if (lhsBottom != rhsBottom)
                return lhsBottom < rhsBottom;

            // 2. memory address (to ensure a unique ordering)
            return lhs < rhs;
        }
    };

    typedef std::set<Entity *, ComparePointers> set_t;
};

#endif
