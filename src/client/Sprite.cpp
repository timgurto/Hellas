#include <cassert>

#include "Client.h"
#include "Sprite.h"
#include "Renderer.h"
#include "TooltipBuilder.h"

#include "../util.h"

extern Renderer renderer;

const std::string Sprite::EMPTY_NAME = "";

Sprite::Sprite(const SpriteType *type, const Point &location):
_yChanged(false),
_type(type),
_location(location),
_toRemove(false){}

Rect Sprite::drawRect() const {
    assert(_type != nullptr);
    return _type->drawRect() + _location;
}

void Sprite::draw(const Client &client) const{
    const Texture &imageToDraw = client.currentMouseOverEntity() == this ? highlightImage() : image();
    if (!imageToDraw)
        return;
    imageToDraw.draw(drawRect() + client.offset());
}

double Sprite::bottomEdge() const{
    if (_type != nullptr)
        return _location.y + _type->drawRect().y + _type->height();
    else
        return _location.y;
}

void Sprite::location(const Point &loc){
    const double oldY = _location.y;
    _location = loc;
    if (_location.y != oldY)
        _yChanged = true;
}

bool Sprite::collision(const Point &p) const{
    return ::collision(p, drawRect());
}

const Texture &Sprite::cursor(const Client &client) const {
    return client.cursorNormal();
}
