#include "Item.h"
#include "Client.h"
#include "Color.h"

Item::Item(const std::string &idArg, const std::string &nameArg, size_t stackSizeArg):
_id(idArg),
_name(nameArg),
_stackSize(stackSizeArg){}

Item::Item(const std::string &idArg):
_id(idArg){}

const std::string &Item::id() const{
    return _id;
}

size_t Item::stackSize() const{
    return _stackSize;
}

const Texture &Item::icon(){
    if (!_icon) {
        _icon = Texture(std::string("Images/") + _id + ".bmp", Color::MAGENTA);
    }
    return _icon;
}

bool Item::operator<(const Item &rhs) const{
    return _id < rhs._id;
}
