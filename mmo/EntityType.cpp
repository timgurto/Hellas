#include "Client.h"
#include "Color.h"
#include "EntityType.h"

EntityType::EntityType(const SDL_Rect &drawRect, const std::string &imageFile):
_drawRect(drawRect),
_image(0){
    if (imageFile != "")
        image(imageFile);
}

EntityType::~EntityType(){
    if (_image)
        SDL_DestroyTexture(_image);
}

void EntityType::image(const std::string &imageFile){
    SDL_Surface *tempSurface = SDL_LoadBMP(imageFile.c_str());
    SDL_SetColorKey(tempSurface, SDL_TRUE, Color::MAGENTA);
    _image = SDL_CreateTextureFromSurface(Client::screen, tempSurface);
    SDL_FreeSurface(tempSurface);

    SDL_QueryTexture(_image, 0, 0, &_drawRect.w, &_drawRect.h);
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
    if (_image) {
        SDL_RenderCopy(Client::screen, _image, 0, &(_drawRect + loc));
    }
}
