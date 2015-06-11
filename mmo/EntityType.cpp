#include "EntityType.h"

#include "Color.h"

SDL_Surface *EntityType::_screen = 0;

EntityType::EntityType(const SDL_Rect &drawRect, const std::string &imageFile):
_drawRect(drawRect),
_image(0){
    if (imageFile != "")
        image(imageFile);
}

EntityType::~EntityType(){
    if (_image)
        SDL_FreeSurface(_image);
}

void EntityType::image(const std::string &imageFile){
    _image = SDL_LoadBMP(imageFile.c_str());

    // These are not strictly necessary for drawing, but might be useful to client code
    _drawRect.w = _image->w;
    _drawRect.h = _image->h;

    SDL_SetColorKey(_image, SDL_TRUE, Color::MAGENTA);
}

const SDL_Rect &EntityType::drawRect() const{
    return _drawRect;
}

int EntityType::width() const{
    return _image ? _image->w : 0;
}

int EntityType::height() const{
    return _image ? _image->h : 0;
}

void EntityType::setScreen(SDL_Surface *screen){
    _screen = screen;
}

void EntityType::drawAt(const Point &loc) const{
    if (_screen && _image) {
        SDL_BlitSurface(_image, 0, _screen, &(_drawRect + loc));
    }
}
