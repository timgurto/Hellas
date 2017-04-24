#include <cassert>

#include "Client.h"
#include "Entity.h"
#include "Renderer.h"
#include "TooltipBuilder.h"

#include "../util.h"

extern Renderer renderer;

const std::string Entity::EMPTY_NAME = "";

Entity::Entity(const EntityType *type, const Point &location):
_yChanged(false),
_type(type),
_location(location),
_toRemove(false){}

Rect Entity::drawRect() const {
    assert(_type != nullptr);
    return _type->drawRect() + _location;
}

void Entity::draw(const Client &client) const{
    const Texture &imageToDraw = client.currentMouseOverEntity() == this ? highlightImage() : image();
    if (!imageToDraw)
        return;
    imageToDraw.draw(drawRect() + client.offset());
}

double Entity::bottomEdge() const{
    if (_type != nullptr)
        return _location.y + _type->drawRect().y + _type->height();
    else
        return _location.y;
}

void Entity::location(const Point &loc){
    const double oldY = _location.y;
    _location = loc;
    if (_location.y != oldY)
        _yChanged = true;
}

bool Entity::collision(const Point &p) const{
    return ::collision(p, drawRect());
}

const Texture &Entity::cursor(const Client &client) const {
    return client.cursorNormal();
}
