// (C) 2015-2016 Tim Gurto

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
_npc(false),
_gatherSound(nullptr){}

ClientObjectType::~ClientObjectType(){
    // TODO: wrap sound functionality in class that properly handeles copies.
    /*if (_gatherSound != nullptr)
        Mix_FreeChunk(_gatherSound);*/
}

void ClientObjectType::gatherSound(const std::string &filename){
    _gatherSound = Mix_LoadWAV(filename.c_str());
}
