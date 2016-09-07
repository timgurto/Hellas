// (C) 2016 Tim Gurto

#ifndef NPC_TYPE_H
#define NPC_TYPE_H

#include "ObjectType.h"

// Describes a class of NPCs, a la the ObjectType class.
class NPCType : public ObjectType{
public:
    NPCType(const std::string &id);

};

#endif
