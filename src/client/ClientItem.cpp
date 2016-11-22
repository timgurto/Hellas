#include "Client.h"
#include "ClientItem.h"

ClientItem::ClientItem(const std::string &id, const std::string &name):
Item(id),
_name(name),
_constructsObject(nullptr){}

void ClientItem::icon(const std::string &filename){
    static const std::string
        prefix = "Images/Items/",
        suffix = ".png";
    _icon = Texture(prefix + filename + suffix, Color::MAGENTA);
}

void ClientItem::gearImage(const std::string &filename){
    static const std::string
        prefix = "Images/Gear/",
        suffix = ".png";
    _gearImage = Texture(prefix + filename + suffix, Color::MAGENTA);
    _drawRect.w = _gearImage.width();
    _drawRect.h = _gearImage.height();
}

const ClientItem *toClientItem(const Item *item){
    return dynamic_cast<const ClientItem *>(item);
}

void ClientItem::draw(const Point &loc) const{
    if (_gearImage)
        _gearImage.draw(_drawRect + loc + Client::_instance->offset());
}
