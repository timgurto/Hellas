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
    if (imageToDraw)
        imageToDraw.draw(drawRect() + client.offset());

    if (shouldDrawName())
        drawName();
}

void Sprite::drawName() const {
    const auto &client = Client::instance();
    const auto nameLabel = Texture{ client.defaultFont(), name(), nameColor() };
    const auto nameOutline = Texture{ client.defaultFont(), name(), Color::PLAYER_NAME_OUTLINE };
    auto namePosition = location() + client.offset();
    namePosition.y -= height();
    namePosition.y -= 16;
    namePosition.x -= nameLabel.width() / 2;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            nameOutline.draw(namePosition + Point(x, y));
    nameLabel.draw(namePosition);
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
