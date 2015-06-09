#include "Item.h"

#include "Color.h"

Item::Item(const std::string &id, const std::string &name):
_id(id),
_name(name),
_icon(0){}

Item::Item(const std::string &id):
_id(id),
_icon(0){}

Item::~Item(){
    if (_icon)
        SDL_FreeSurface(_icon);
}

SDL_Surface *Item::getIcon(){
    if (!_icon) {
        std::string filename = std::string("Images/") + _id + ".bmp";
        _icon = SDL_LoadBMP(filename.c_str());
        SDL_SetColorKey(_icon, SDL_TRUE, Color::MAGENTA);
    }
    return _icon;
}

bool Item::operator<(const Item &rhs) const{
    return _id < rhs._id;
}
