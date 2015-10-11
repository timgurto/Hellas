// (C) 2015 Tim Gurto

#ifndef CLIENT_OBJECT_TYPE_H
#define CLIENT_OBJECT_TYPE_H

#include <SDL_mixer.h>
#include <string>

#include "EntityType.h"
#include "Point.h"
#include "Texture.h"
#include "util.h"

// Describes a class of Entities, the "instances" of which share common properties
class ClientObjectType : public EntityType{
    std::string _id;
    std::string _name;
    bool _canGather; // Whether this represents objects that can be gathered
    Mix_Chunk *_gatherSound;

public:
    ClientObjectType(const std::string &id);
    ~ClientObjectType();

    const std::string &name() const { return _name; }
    void name(const std::string &s) { _name = s; }
    bool canGather() const { return _canGather; }
    void canGather(bool b) { _canGather = b; }
    void gatherSound(const std::string &filename);
    Mix_Chunk *gatherSound() const { return _gatherSound; }

    bool operator<(const ClientObjectType &rhs) const { return _id < rhs._id; }
};

#endif
