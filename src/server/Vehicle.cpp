#include "Vehicle.h"
#include "VehicleType.h"

const double Vehicle::SPEED = 50;

Vehicle::Vehicle(const VehicleType *type, const Point &loc):
_driver(""),
Object(type, loc){}
