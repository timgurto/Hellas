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

// Shouldn't usually be called directly; instead call Client::setClientLocation().
void Entity::locationInner(const Point &location){
    _location = location;
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
