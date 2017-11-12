#include "Vehicle.h"
#include "VehicleType.h"

Vehicle::Vehicle(const VehicleType *type, const Point &loc):
Object(type, loc){}
