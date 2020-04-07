#include "Vehicle.h"

#include "VehicleType.h"

Vehicle::Vehicle(const VehicleType *type, const MapPoint &loc)
    : Object(type, loc) {}

bool Vehicle::shouldMoveWhereverRequested() const {
  auto isDriving = !_driver.empty();
  return isDriving;
}
