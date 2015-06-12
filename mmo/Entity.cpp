#include "Entity.h"

Entity::Entity(const EntityType &type, const Point &location):
_type(type),
_location(location){}

bool Entity::operator<(const Entity &rhs) const{
    double
        lhsBottom = bottomEdge(),
        rhsBottom = rhs.bottomEdge();

    // 1. location
    if (lhsBottom != rhsBottom)
        return lhsBottom < rhsBottom;

    // 2. memory address (to ensure a unique ordering)
    return this < &rhs;
}

const Point &Entity::location() const{
    return _location;
}

SDL_Rect Entity::drawRect() const {
    return _type.drawRect() + _location;
}

int Entity::width() const{
    return _type.width();
}

int Entity::height() const{
    return _type.height();
}

void Entity::draw() const{
    _type.drawAt(_location);
}

double Entity::bottomEdge() const{
    return _location.y + _type.drawRect().y + _type.height();
}

void Entity::setLocation(set_t &entitiesSet, const Point &newLocation){
    double oldY = _location.y;

    // Remove entity from set
    if (oldY != newLocation.y) {
        set_t::iterator it = entitiesSet.find(this);
        if (it != entitiesSet.end())
            entitiesSet.erase(it);
    }

    // Update location
    _location = newLocation;

    // Add entity to set
    if (oldY != newLocation.y) {
        entitiesSet.insert(this);
    }
}
