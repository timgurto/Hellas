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

    // To pick up objects of this type, a user must have an item of the following class
    std::string _pickUpReq;

public:
    ObjectType(const std::string &id, Uint32 actionTime = 0, size_t wood = 0);

    size_t wood() const { return _wood; }
    void wood(size_t qty) { _wood = qty; }
    const std::string &pickUpReq() const { return _pickUpReq; }
    void pickUpReq(const std::string &req) { _pickUpReq = req; }

    const std::string &id() const { return _id; }
    Uint32 actionTime() const { return _actionTime; }

    bool operator<(const ObjectType &rhs) const { return _id < rhs._id; }
};

#endif
