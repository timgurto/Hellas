#ifndef VEHICLE_H
#define VEHICLE_H

#include "Object.h"

class VehicleType;

class Vehicle : public Object{
    std::string _driver;
    static const double SPEED;

public:
    Vehicle(const VehicleType *type, const Point &loc);
    virtual ~Vehicle(){}

    const std::string &driver() const { return _driver; }
    void driver(const std::string &username) { _driver = username; }

    virtual char classTag() const override { return 'v'; }
    virtual double speed() const override { return SPEED; }
};

#endif
