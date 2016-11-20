#include <SDL.h>
#include <SDL_mixer.h>

#include "ClientObjectType.h"
#include "../Color.h"

ClientObjectType::ClientObjectType(const std::string &id):
_id(id),
_canGather(false),
_canDeconstruct(false),
_containerSlots(0),
_merchantSlots(0),
_gatherSound(nullptr),
_gatherParticles(nullptr)
{}

ClientObjectType::~ClientObjectType(){
    // TODO: wrap sound functionality in class that properly handles copies.
    /*if (_gatherSound != nullptr)
        Mix_FreeChunk(_gatherSound);*/
}

void ClientObjectType::gatherSound(const std::string &filename){
    _gatherSound = Mix_LoadWAV(filename.c_str());
}
