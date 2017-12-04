#ifndef CLIENT_VEHICLE_TYPE_H
#define CLIENT_VEHICLE_TYPE_H

#include "ClientObjectType.h"
#include "ClientObjectType.h"

class ClientVehicleType : public ClientObjectType{
    bool _drawDriver;
    ScreenPoint _driverOffset; // Where to draw the driver
public:
    ClientVehicleType(const std::string &id);
    virtual char classTag() const override { return 'v'; }

    bool drawDriver() const { return _drawDriver; }
    void drawDriver(bool b) { _drawDriver = b; }
    const ScreenPoint &driverOffset() const { return _driverOffset; }
    void driverOffset(const ScreenPoint &p) { _driverOffset = p; }
};

#endif
