#include <SDL.h>
#include <SDL_mixer.h>

#include "ClientObjectType.h"
#include "TooltipBuilder.h"
#include "../Color.h"

ClientObjectType::ClientObjectType(const std::string &id):
_id(id),
_canGather(false),
_canDeconstruct(false),
_containerSlots(0),
_merchantSlots(0),
_gatherSound(nullptr),
_gatherParticles(nullptr),
_materialsTooltip(nullptr)
{}

ClientObjectType::~ClientObjectType(){
    // TODO: wrap sound functionality in class that properly handles copies.
    /*if (_gatherSound != nullptr)
        Mix_FreeChunk(_gatherSound);*/
    if (_materialsTooltip != nullptr)
        delete _materialsTooltip;
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
