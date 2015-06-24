#include "Item.h"
#include "Client.h"
#include "Color.h"

Item::Item(const std::string &idArg, const std::string &nameArg, size_t stackSizeArg):
_id(idArg),
_name(nameArg),
_stackSize(stackSizeArg),
_icon(std::string("Images/") + _id + ".bmp", Color::MAGENTA){}

Item::Item(const std::string &idArg):
_id(idArg){}
