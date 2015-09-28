// (C) 2015 Tim Gurto

#ifndef OBJECT_TYPE_H
#define OBJECT_TYPE_H

#include <SDL.h>
#include <string>

// Describes a class of Objects, the "instances" of which share common properties
class ObjectType{
    std::string _id;
    Uint32 _actionTime;
    size_t _wood; // TODO: abstract _resources, probably map<item-id, pair<mean,sd>>

    // To gather objects of this type, a user must have an item of the following class
    std::string _gatherReq;

public:
    ObjectType(const std::string &id);

    void actionTime(Uint32 t) { _actionTime = t; }
    size_t wood() const { return _wood; }
    void wood(size_t qty) { _wood = qty; }
    const std::string &gatherReq() const { return _gatherReq; }
    void gatherReq(const std::string &req) { _gatherReq = req; }

    const std::string &id() const { return _id; }
    Uint32 actionTime() const { return _actionTime; }

    bool operator<(const ObjectType &rhs) const { return _id < rhs._id; }
};

#endif
