#include "Client.h"
#include "ClientItem.h"
#include "../XmlReader.h"

std::map<int, size_t> ClientItem::gearDrawOrder;
std::vector<Point> ClientItem::gearOffsets(Client::GEAR_SLOTS);

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
}

const ClientItem *toClientItem(const Item *item){
    return dynamic_cast<const ClientItem *>(item);
}

void ClientItem::draw(const Point &loc) const{
    if (_gearSlot <= Client::GEAR_SLOTS && _gearImage){
        Point drawLoc =
            _drawLoc +                   // The item's offset
            gearOffsets[_gearSlot] +     // The slot's offset
            loc +                        // The avatar's location
            Client::_instance->offset(); // The overall map offset
        _gearImage.draw(drawLoc);
    }
}

void ClientItem::init(){
    XmlReader xr("client-config.xml");
    auto elem = xr.findChild("gearDisplay");
    if (elem == nullptr)
        return;
    for (auto slot : xr.getChildren("slot", elem)){
        size_t slotNum;
        if (!xr.findAttr(slot, "num", slotNum))
            continue;
        
        // Offsets
        xr.findAttr(slot, "midX", gearOffsets[slotNum].x);
        xr.findAttr(slot, "midY", gearOffsets[slotNum].y);
        
        // Draw order.  Without this, gear for this slot won't be drawn.
        size_t order;
        if (xr.findAttr(slot, "drawOrder", order))
            gearDrawOrder[order] = slotNum;
    }
}
