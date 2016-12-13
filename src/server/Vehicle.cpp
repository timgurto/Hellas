#include "Vehicle.h"
#include "VehicleType.h"

Vehicle::Vehicle(const VehicleType *type, const Point &loc):
_driver(""),
Object(type, loc){}
