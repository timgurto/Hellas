// (C) 2015 Tim Gurto

#include <SDL.h>

#include "Color.h"
#include "ClientObjectType.h"

ClientObjectType::ClientObjectType(const SDL_Rect &drawRect, const std::string &imageFile,
                       const std::string &id, const std::string &name, bool canGather):
EntityType(drawRect, imageFile),
_id(id),
_name(name),
_canGather(canGather){}

ClientObjectType::ClientObjectType(const std::string &id):
_id(id){}
