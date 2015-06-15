#include <SDL.h>

#include "Client.h"
#include "Color.h"
#include "EntityType.h"

EntityType::EntityType(const SDL_Rect &drawRect, const std::string &imageFile):
_drawRect(drawRect),
_image(imageFile){
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
}

void EntityType::image(const std::string &imageFile){
    _image = Texture(imageFile, Color::MAGENTA);
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
}

const SDL_Rect &EntityType::drawRect() const{
    return _drawRect;
}

int EntityType::width() const{
    return _image ? _drawRect.w : 0;
}

int EntityType::height() const{
    return _image ? _drawRect.h : 0;
}

void EntityType::drawAt(const Point &loc) const{
    _image.draw(_drawRect + loc);
}
