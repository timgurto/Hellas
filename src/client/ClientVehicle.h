#ifndef CLIENT_VEHICLE_H
#define CLIENT_VEHICLE_H

#include "ClientObject.h"
#include "ClientVehicleType.h"

class ClientVehicle : public ClientObject{

public:
    ClientVehicle(size_t serial, const ClientVehicleType *type = nullptr,
            const Point &loc = Point());
    virtual ~ClientVehicle(){}

    virtual char classTag() const override { return 'v'; }

    static void ClientVehicle::mountOrDismount(void *object);
};

#endif
