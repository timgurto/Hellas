// (C) 2015 Tim Gurto

#ifndef CLIENT_OBJECT_TYPE_H
#define CLIENT_OBJECT_TYPE_H

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

public:
    ClientObjectType(const SDL_Rect &drawRect = makeRect(), const std::string &imageFile = "",
                     const std::string &id = "", const std::string &name = "",
                     bool canGather = false);
    ClientObjectType(const std::string &id); // For set lookup

    bool canGather() const { return _canGather; }
    const std::string &name() const { return _name; }

    bool operator<(const ClientObjectType &rhs) const { return _id < rhs._id; }
};

#endif
