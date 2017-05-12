#ifndef VEHICLE_TYPE_H
#define VEHICLE_TYPE_H

#include "objects/ObjectType.h"

class User;

class VehicleType : public ObjectType{
public:
    VehicleType(const std::string &id): ObjectType(id) {}
    virtual ~VehicleType(){}
    
    virtual char classTag() const override { return 'v'; }
};

#endif
