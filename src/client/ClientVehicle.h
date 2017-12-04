#ifndef CLIENT_VEHICLE_H
#define CLIENT_VEHICLE_H

#include "ClientObject.h"
#include "ClientVehicleType.h"

class Avatar;

class ClientVehicle : public ClientObject{
    const Avatar *_driver;

public:
    ClientVehicle(size_t serial, const ClientVehicleType *type = nullptr,
        const MapPoint &loc = MapPoint{});
    virtual ~ClientVehicle(){}

    const Avatar *driver() const { return _driver; }
    void driver(const Avatar *p) { _driver = p; }

    virtual char classTag() const override { return 'v'; }
    virtual void draw(const Client &client) const override;

    static void ClientVehicle::mountOrDismount(void *object);
};

#endif
