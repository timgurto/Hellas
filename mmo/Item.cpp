#include "Item.h"

#include "Color.h"

Item::Item(const std::string &idArg, const std::string &nameArg, size_t stackSizeArg):
id(idArg),
name(nameArg),
stackSize(stackSizeArg),
icon(0){}

Item::Item(const std::string &idArg):
id(idArg),
icon(0){}

Item::~Item(){
    if (icon)
        SDL_FreeSurface(icon);
}

SDL_Surface *Item::getIcon(){
    if (!icon) {
        std::string filename = std::string("Images/") + id + ".bmp";
        icon = SDL_LoadBMP(filename.c_str());
        SDL_SetColorKey(icon, SDL_TRUE, Color::MAGENTA);
    }
    return icon;
}

bool Item::operator<(const Item &rhs) const{
    return id < rhs.id;
}
