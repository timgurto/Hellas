#include "Item.h"

#include "Color.h"

Item::Item(const std::string &idArg, const std::string &nameArg, size_t stackSizeArg):
_id(idArg),
_name(nameArg),
_stackSize(stackSizeArg),
_icon(0){}

Item::Item(const std::string &idArg):
_id(idArg),
_icon(0){}

Item::~Item(){
    if (_icon)
        SDL_FreeSurface(_icon);
}

const std::string &Item::id() const{
    return _id;
}

size_t Item::stackSize() const{
    return _stackSize;
}

SDL_Surface *Item::icon(){
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
