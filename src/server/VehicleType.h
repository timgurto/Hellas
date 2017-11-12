#ifndef VEHICLE_TYPE_H
#define VEHICLE_TYPE_H

#include "objects/ObjectType.h"

class User;

class VehicleType : public ObjectType{
public:
    VehicleType(const std::string &id) : ObjectType(id) { _baseStats.speed = 50; }
    virtual ~VehicleType(){}
    
    virtual char classTag() const override { return 'v'; }
};

#endif
