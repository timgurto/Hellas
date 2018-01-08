#include "Client.h"
#include "ClientItem.h"
#include "Tooltip.h"
#include "../XmlReader.h"

std::map<int, size_t> ClientItem::gearDrawOrder;
std::vector<ScreenPoint> ClientItem::gearOffsets(Client::GEAR_SLOTS);

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

static ScreenPoint toScreenPoint(const MapPoint &rhs) {
    return{ toInt(rhs.x), toInt(rhs.y) };
}

void ClientItem::draw(const MapPoint &loc) const{
    if (_gearSlot <= Client::GEAR_SLOTS && _gearImage){
        ScreenPoint drawLoc =
            _drawLoc +                   // The item's offset
            gearOffsets[_gearSlot] +     // The slot's offset
            toScreenPoint(loc) +      // The avatar's location
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

const Texture &ClientItem::tooltip() const{
    if (_tooltip)
        return _tooltip;

    const auto &client = *Client::_instance;

    Tooltip tb;
    tb.setColor(Color::ITEM_NAME);
    tb.addLine(_name);

    // Gear slot/stats
    if (_gearSlot != Client::GEAR_SLOTS){
        tb.addGap();
        tb.setColor(Color::ITEM_STATS);
        tb.addLine("Gear: " + Client::GEAR_SLOT_NAMES[_gearSlot]);
        tb.addLines(_stats.toStrings());
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

    // Spell
    if (castsSpellOnUse()) {
        auto it = client._spells.find(spellToCastOnUse());
        if (it == client._spells.end()) {
            client.debug() << Color::FAILURE << "Can't find spell: " << spellToCastOnUse() << Log::endl;
        } else {
            tb.setColor(Color::ITEM_STATS);
            tb.addLine("Right-click: "s + it->second->createEffectDescription());
        }
    }


    _tooltip = tb.publish();
    return _tooltip;
}

void ClientItem::sounds(const std::string &id){
    const Client &client = *Client::_instance;
    _sounds = client.findSoundProfile(id);
}

bool ClientItem::canUse() const {
    return
        _constructsObject != nullptr ||
        castsSpellOnUse();
}
