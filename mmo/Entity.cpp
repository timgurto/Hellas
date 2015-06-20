#include "Entity.h"

#include "util.h"

Entity::Entity(const EntityType &type, const Point &location):
_type(type),
_location(location),
_yChanged(false){}

SDL_Rect Entity::drawRect() const {
    return _type.drawRect() + _location;
}

void Entity::draw(const Client &client) const{
    _type.drawAt(_location);
}

double Entity::bottomEdge() const{
    return _location.y + _type.drawRect().y + _type.height();
}

void Entity::location(const Point &loc){
    double oldY = _location.y;
    _location = loc;
    if (_location.y != oldY)
        _yChanged = true;
}

bool Entity::collision(const Point &p) const{
    return ::collision(p, drawRect());
}
