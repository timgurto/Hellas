#include <cassert>

#include <SDL.h>
#include <SDL_mixer.h>

#include "Client.h"
#include "ClientObjectType.h"
#include "Surface.h"
#include "TooltipBuilder.h"
#include "../Color.h"

ClientObjectType::ClientObjectType(const std::string &id):
EntityType(Rect(), id),
_id(id),
_canGather(false),
_canDeconstruct(false),
_containerSlots(0),
_merchantSlots(0),
_gatherSound(nullptr),
_gatherParticles(nullptr),
_materialsTooltip(nullptr),
_transformTime(0)
{}

ClientObjectType::~ClientObjectType(){
    // TODO: wrap sound functionality in class that properly handles copies.
    /*if (_gatherSound != nullptr)
        Mix_FreeChunk(_gatherSound);*/
    if (_materialsTooltip != nullptr){
        delete _materialsTooltip;
        _materialsTooltip = nullptr;
    }
}

void ClientObjectType::gatherSound(const std::string &filename){
    _gatherSound = Mix_LoadWAV(filename.c_str());
}

const Texture &ClientObjectType::materialsTooltip() const{
    if (_materialsTooltip == nullptr){
        TooltipBuilder tb;
        tb.setColor(Color::ITEM_NAME);
        tb.addLine(_name);
        tb.addGap();
        tb.setColor(Color::ITEM_STATS);
        tb.addLine("Materials:");
        for (const auto &material : _materials){
            const ClientItem &item = *dynamic_cast<const ClientItem *>(material.first);
            tb.addLine(makeArgs(material.second) + "x " + item.name());
        }
        _materialsTooltip = new Texture(tb.publish());
    }
    return *_materialsTooltip;
}

const ClientObjectType::ImageSet &ClientObjectType::getProgressImage(ms_t timeRemaining) const{
    double progress = 1 - (1.0 * timeRemaining / _transformTime);
    size_t numFrames = _transformImages.size();
    int index = static_cast<int>(progress * numFrames) - 1;
    if (_transformImages.empty())
        index = 0;
    index = max<int>(index, 0);
    index = min<int>(index, _transformImages.size()); // Progress may be 100% due to server delay.
    if (index == 0)
        return _images;
    return _transformImages[index-1];
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
