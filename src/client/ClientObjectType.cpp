#include <cassert>

#include <SDL.h>

#include "Client.h"
#include "ClientObjectType.h"
#include "SoundProfile.h"
#include "Surface.h"
#include "TooltipBuilder.h"
#include "../Color.h"

ClientObjectType::ClientObjectType(const std::string &id):
    SpriteType({}, id),
_id(id),
_canGather(false),
_canDeconstruct(false),
_containerSlots(0),
_merchantSlots(0),
_sounds(nullptr),
_gatherParticles(nullptr),
_materialsTooltip(nullptr),
_transformTime(0)
{}

ClientObjectType::~ClientObjectType(){
    if (_materialsTooltip != nullptr){
        delete _materialsTooltip;
        _materialsTooltip = nullptr;
    }
}

const Texture &ClientObjectType::constructionTooltip() const{
    const auto &client = *Client::_instance;

    if (_materialsTooltip == nullptr){
        Tooltip tb;
        tb.setColor(Color::ITEM_NAME);
        tb.addLine(_name);

        auto gapDrawn = false;
        if (canGather()) {
            if (!gapDrawn) { gapDrawn = true; tb.addGap(); }
            std::string text = "Gatherable";
            if (!gatherReq().empty())
                text += " (requires " + client.tagName(gatherReq()) + ")";
            tb.addLine(text);
        }

        if (canDeconstruct()) {
            if (!gapDrawn) { gapDrawn = true; tb.addGap(); }
            tb.addLine("Can pick up as item");
        }

        if (containerSlots() > 0) {
            if (!gapDrawn) { gapDrawn = true; tb.addGap(); }
            tb.addLine("Container: " + toString(containerSlots()) + " slots");
        }

        if (merchantSlots() > 0) {
            if (!gapDrawn) { gapDrawn = true; tb.addGap(); }
            tb.addLine("Merchant: " + toString(merchantSlots()) + " slots");
        }

        // Tags
        if (hasTags()) {
            tb.addGap();
            tb.setColor(Color::ITEM_TAGS);
            for (const std::string &tag : tags())
                tb.addLine(client.tagName(tag));
        }

        tb.addGap();
        tb.setColor(Color::ITEM_STATS);
        tb.addLine("Construction materials:");
        for (const auto &material : _materials){
            const ClientItem &item = *dynamic_cast<const ClientItem *>(material.first);
            tb.addLine(makeArgs(material.second) + "x " + item.name());
        }

        if (! _constructionReq.empty()){
            tb.addGap();
            tb.addLine("Requires tool: " + client.tagName(_constructionReq));
        }

        _materialsTooltip = new Texture(tb.publish());
    }
    return *_materialsTooltip;
}

const ClientObjectType::ImageSet &ClientObjectType::getProgressImage(ms_t timeRemaining) const{
    double progress = 1 - (1.0 * timeRemaining / _transformTime);
    size_t numFrames = _transformImages.size();
    int index = static_cast<int>(progress * (numFrames+1)) - 1;
    if (_transformImages.empty())
        index = -1;
    index = max<int>(index, -1);
    index = min<int>(index, _transformImages.size()-1); // Progress may be 100% due to server delay.
    if (index == -1)
        return _images;
    return _transformImages[index];
}

void ClientObjectType::corpseImage(const std::string &filename){
    _corpseImage = Texture(filename, Color::MAGENTA);
    if (!_corpseImage)
        return;

    // Set corpse highlight image
    Surface corpseHighlightSurface(filename, Color::MAGENTA);
    if (!corpseHighlightSurface)
        return;
    corpseHighlightSurface.swapColors(Color::OUTLINE, Color::HIGHLIGHT_OUTLINE);
    _corpseHighlightImage = Texture(corpseHighlightSurface);
}

void ClientObjectType::addTransformImage(const std::string &filename){
    _transformImages.push_back(ImageSet("Images/Objects/" + filename + ".png"));
}

void ClientObjectType::addMaterial(const ClientItem *item, size_t qty) {
    _materials.set(item, qty);
    if (!_constructionImage.normal)
        _constructionImage = ImageSet("Images/Objects/" + _id + "-construction.png");
}

ClientObjectType::ImageSet::ImageSet(const std::string &filename){
    Surface surface(filename, Color::MAGENTA);
    normal = Texture(surface);
    surface.swapColors(Color::OUTLINE, Color::HIGHLIGHT_OUTLINE);
    highlight = Texture(surface);
}

void ClientObjectType::sounds(const std::string &id){
    const Client &client = *Client::_instance;
    _sounds = client.findSoundProfile(id);
}

void ClientObjectType::calculateAndInitStrength(){
    if (_strength.item == nullptr)
        return;
    maxHealth(_strength.item->strength() * _strength.quantity);
}
