// (C) 2015 Tim Gurto

#include <SDL.h>

#include "../Color.h"
#include "EntityType.h"

EntityType::EntityType(const Rect &drawRect, const std::string &imageFile):
_image(imageFile, Color::MAGENTA),
_drawRect(drawRect),
_isFlat(false){
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
}

void EntityType::image(const std::string &imageFile){
    _image = Texture(imageFile, Color::MAGENTA);
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
}

void EntityType::drawAt(const Point &loc) const{
    _image.draw(_drawRect + loc);
}
