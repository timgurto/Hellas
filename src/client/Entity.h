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

    bool _toRemove; // No longer draw or update, and remove when possible.

    static const std::string EMPTY_NAME;

protected:
    mutable Texture _tooltip;

public:
    Entity(const EntityType *type, const Point &location);
    virtual ~Entity(){}

    const Point &location() const { return _location; }
    void location(const Point &loc); // yChanged() should be checked after changing location.
    virtual Rect drawRect() const;
    px_t width() const { return _type->width(); }
    px_t height() const { return _type->height(); }
    bool yChanged() const { return _yChanged; }
    void yChanged(bool val) { _yChanged = val; }
    const EntityType *type() const { return _type; }
    void type(const EntityType *et) { _type = et; }
    virtual const Texture &image() const { return _type->image(); }
    virtual const Texture &highlightImage() const { return _type->highlightImage(); }
    void markForRemoval() { _toRemove = true; }
    bool markedForRemoval() const { return _toRemove; }
    virtual bool isFlat() const { return _type->isFlat(); }

    virtual char classTag() const { return 'e'; }

    virtual void draw(const Client &client) const;
    virtual void update(double delta) {}
    virtual void onLeftClick(Client &client) {}
    virtual void onRightClick(Client &client) {}
    virtual const Texture &cursor(const Client &client) const;
    virtual const Texture &tooltip() const { return _tooltip; }
    virtual const std::string &name() const { return EMPTY_NAME; }
    void refreshTooltip() const { _tooltip = Texture(); }

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
