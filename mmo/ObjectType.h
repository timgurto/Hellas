// (C) 2015 Tim Gurto

#ifndef OBJECT_TYPE_H
#define OBJECT_TYPE_H

#include <SDL.h>
#include <string>

// Describes a class of Objects, the "instances" of which share common properties
class ObjectType{
    std::string _id;
    Uint32 _actionTime;

public:
    ObjectType(const std::string &id, Uint32 actionTime = 0);
};

#endif
