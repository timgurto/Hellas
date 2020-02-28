#include "ClientVehicleType.h"

#include "Client.h"

ClientVehicleType::ClientVehicleType(const std::string &id)
    : ClientObjectType(id),
      _drawDriver(false),
      _driverOffset(),
      _speed(Client::MOVEMENT_SPEED) {}
