// (C) 2015 Tim Gurto

#include <cassert>

#include "Client.h"
#include "Entity.h"
#include "Renderer.h"
#include "TooltipBuilder.h"

#include "util.h"

extern Renderer renderer;

Entity::Entity(const EntityType *type, const Point &location):
_yChanged(false),
_type(type),
_location(location),
_needsTooltipRefresh(false){}

SDL_Rect Entity::drawRect() const {
    return _type->drawRect() + _location;
}

void Entity::draw(const Client &client) const{
    assert(_type);
    _type->drawAt(_location + client.offset());
    if (isDebug()) {
        renderer.setDrawColor(Color::YELLOW);
        renderer.fillRect(makeRect(_location.x + client.offset().x,
                                   _location.y + client.offset().y - 1,
                                   1, 3));
        renderer.fillRect(makeRect(_location.x + client.offset().x - 1,
                                   _location.y + client.offset().y,
                                   3, 1));
    }
}

double Entity::bottomEdge() const{
    if (_type)
        return _location.y + _type->drawRect().y + _type->height();
    else
        return _location.y;
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

void Entity::refreshTooltip(const Client &client){
    _needsTooltipRefresh = false;

    std::vector<std::string> textStrings = getTooltipMessages(client);
    if (textStrings.empty()) {
        _tooltip = Texture();
        return;
    }

    std::vector<std::string>::const_iterator it = textStrings.begin();
    TooltipBuilder tb;
    tb.setColor(Color::WHITE);
    tb.addLine(*it);
    tb.setColor();
    ++it;

    if (it != textStrings.end())
        tb.addGap();

    for (; it != textStrings.end(); ++it)
        tb.addLine(*it);

    _tooltip = tb.publish();
}
