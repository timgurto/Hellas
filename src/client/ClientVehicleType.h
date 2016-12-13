#ifndef CLIENT_VEHICLE_TYPE_H
#define CLIENT_VEHICLE_TYPE_H

#include "ClientObjectType.h"
#include "ClientObjectType.h"

class ClientVehicleType : public ClientObjectType{
public:
    ClientVehicleType(const std::string &id): ClientObjectType(id) {}
    virtual char classTag() const override { return 'v'; }
};

#endif
