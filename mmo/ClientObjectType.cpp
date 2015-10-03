// (C) 2015 Tim Gurto

#include <SDL.h>
#include <SDL_mixer.h>

#include "Color.h"
#include "ClientObjectType.h"

ClientObjectType::ClientObjectType(const std::string &id):
_id(id),
_gatherSound(0){}

ClientObjectType::~ClientObjectType(){
    // TODO: wrap sound functionality in class that properly handeles copies.
    /*if (_gatherSound)
        Mix_FreeChunk(_gatherSound);*/
}

void ClientObjectType::gatherSound(const std::string &filename){
    _gatherSound = Mix_LoadWAV(filename.c_str());
}
