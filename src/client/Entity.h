// (C) 2015 Tim Gurto

#ifndef ENTITY_H
#define ENTITY_H

#include <set>
#include <vector>

#include "EntityType.h"
#include "Texture.h"
#include "../Point.h"

class Client;

// Handles the graphical and UI side of in-game objects  Abstract class
class Entity{
    bool _yChanged; // y co-ordinate has changed, and the entity must be reordered.
    const EntityType *_type;
    Point _location;
    bool _needsTooltipRefresh;
    virtual std::vector<std::string> getTooltipMessages(const Client &client) const {
        return std::vector<std::string>();
    }

protected:
    Texture _tooltip;

public:
    Entity(const EntityType *type, const Point &location);
    virtual ~Entity(){}

    const Point &location() const { return _location; }
    void location(const Point &loc); // yChanged() should be checked after changing location.
    Rect drawRect() const;
    int width() const { return _type->width(); }
    int height() const { return _type->height(); }
    bool yChanged() const { return _yChanged; }
    void yChanged(bool val) { _yChanged = val; }
    const EntityType *type() const { return _type; }
    bool needsTooltipRefresh() { return _needsTooltipRefresh; }
    const Texture &tooltip() const { return _tooltip; }

    virtual void draw(const Client &client) const;
    virtual void update(double delta) {}
    virtual void onLeftClick(Client &client) {}
    virtual void onRightClick(Client &client) {}

    //Class identifiers
    virtual bool isObject(){ return false; }

    void refreshTooltip(const Client &client);

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
