#include "Item.h"
#include "Client.h"
#include "Color.h"

Item::Item(const std::string &idArg, const std::string &nameArg, size_t stackSizeArg):
_id(idArg),
_name(nameArg),
_stackSize(stackSizeArg){}

Item::Item(const std::string &idArg):
_id(idArg){}

const Texture &Item::icon(){
    if (!_icon) {
        _icon = Texture(std::string("Images/") + _id + ".bmp", Color::MAGENTA);
    }
    return _icon;
}
