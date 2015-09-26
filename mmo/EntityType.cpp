// (C) 2015 Tim Gurto

#include <SDL.h>

#include "Color.h"
#include "EntityType.h"

EntityType::EntityType(const SDL_Rect &drawRect, const std::string &imageFile,
                       const std::string &id, const std::string &name, bool canGather):
_drawRect(drawRect),
_id(id),
_name(name),
_image(imageFile, Color::MAGENTA),
_canGather(canGather){
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
}

EntityType::EntityType(const std::string &id):
    _id(id){}

void EntityType::image(const std::string &imageFile){
    _image = Texture(imageFile, Color::MAGENTA);
    _drawRect.w = _image.width();
    _drawRect.h = _image.height();
}

void EntityType::drawAt(const Point &loc) const{
    _image.draw(_drawRect + loc);
}
