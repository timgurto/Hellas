#include "Client.h"
#include "ClientItem.h"
#include "TooltipBuilder.h"
#include "../XmlReader.h"

std::map<int, size_t> ClientItem::gearDrawOrder;
std::vector<Point> ClientItem::gearOffsets(Client::GEAR_SLOTS);

ClientItem::ClientItem(const std::string &id, const std::string &name):
Item(id),
_name(name),
_constructsObject(nullptr),
_sounds(nullptr)
{}

void ClientItem::icon(const std::string &filename){
    static const std::string
        prefix = "Images/Items/",
        suffix = ".png";
    _icon = Texture(prefix + filename + suffix);
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

std::string multiplicativeToString(double d){
    d -= 1;
    d *= 100;
    return toString(toInt(d)) + "%";
}

const Texture &ClientItem::tooltip() const{
    if (_tooltip)
        return _tooltip;

    const auto &client = *Client::_instance;

    TooltipBuilder tb;
    tb.setColor(Color::ITEM_NAME);
    tb.addLine(_name);

    // Gear slot/stats
    if (_gearSlot != Client::GEAR_SLOTS){
        tb.addGap();
        tb.setColor(Color::ITEM_STATS);
        tb.addLine("Gear: " + Client::GEAR_SLOT_NAMES[_gearSlot]);
        if (_stats.health > 0)
            tb.addLine("+" + toString(_stats.health) + " health");
        if (_stats.energy > 0)
            tb.addLine("+" + toString(_stats.energy) + " energy");
        if (_stats.hps > 0)
            tb.addLine("+" + toString(_stats.hps) + " health per second");
        if (_stats.eps > 0)
            tb.addLine("+" + toString(_stats.eps) + " energy per second");
        if (_stats.hit > 0)
            tb.addLine("+" + toString(_stats.hit) + "% hit");
        if (_stats.crit > 0)
            tb.addLine("+" + toString(_stats.crit) + "% crit");
        if (_stats.magicDamage > 0)
            tb.addLine("+" + toString(_stats.magicDamage) + " magic damage");
        if (_stats.airResist > 0)
            tb.addLine("+" + toString(_stats.airResist) + "% air resistance");
        if (_stats.earthResist > 0)
            tb.addLine("+" + toString(_stats.earthResist) + "% earth resistance");
        if (_stats.fireResist > 0)
            tb.addLine("+" + toString(_stats.fireResist) + "% fire resistance");
        if (_stats.waterResist > 0)
            tb.addLine("+" + toString(_stats.waterResist) + "% water resistance");
        if (_stats.attack > 0)
            tb.addLine("+" + toString(_stats.attack) + " attack");
        if (_stats.attackTime != 1)
            tb.addLine("+" + multiplicativeToString(1/_stats.attackTime) + " attack speed");
        if (_stats.speed != 1)
            tb.addLine("+" + multiplicativeToString(_stats.speed) + " run speed");
    }

    // Tags
    if (hasTags()){
        tb.addGap();
        tb.setColor(Color::ITEM_TAGS);
        for (const std::string &tag : tags())
            tb.addLine(client.tagName(tag));
    }
    
    // Construction
    if (_constructsObject != nullptr){
        tb.addGap();
        tb.setColor(Color::ITEM_INSTRUCTIONS);
        tb.addLine(std::string("Right-click to place ") + _constructsObject->name());
        if (!_constructsObject->constructionReq().empty())
        tb.addLine("(Requires " + client.tagName(_constructsObject->constructionReq()) + ")");

        // Vehicle?
        if (_constructsObject->classTag() == 'v'){
            tb.setColor(Color::ITEM_STATS);
            tb.addLine("  Vehicle");
        }

        if (_constructsObject->containerSlots() > 0){
            tb.setColor(Color::ITEM_STATS);
            tb.addLine("  Container: " + toString(_constructsObject->containerSlots()) + " slots");
        }

        if (_constructsObject->merchantSlots() > 0){
            tb.setColor(Color::ITEM_STATS);
            tb.addLine("  Merchant: " + toString(_constructsObject->merchantSlots()) + " slots");
        }

        // Tags
        if (_constructsObject->hasTags()){
            tb.setColor(Color::ITEM_TAGS);
            for (const std::string &tag : _constructsObject->tags())
                tb.addLine("  " + client.tagName(tag));
        }
    }


    _tooltip = tb.publish();
    return _tooltip;
}

void ClientItem::sounds(const std::string &id){
    const Client &client = *Client::_instance;
    _sounds = client.findSoundProfile(id);
}
