#include "Vehicle.h"

#include "Server.h"
#include "VehicleType.h"

Vehicle::Vehicle(const VehicleType *type, const MapPoint &loc)
    : Object(type, loc) {}

bool Vehicle::shouldMoveWhereverRequested() const {
  auto isDriving = !_driver.empty();
  return isDriving;
}

void Vehicle::onDeath() {
  auto pDriver = Server::instance().getUserByName(_driver);
  if (pDriver) pDriver->driving({});

  _driver.clear();

  Object::onDeath();
}
