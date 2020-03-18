#include "ClientVehicleType.h"

#include "Client.h"

ClientVehicleType::ClientVehicleType(const std::string& id)
    : ClientObjectType(id),
      _drawDriver(false),
      _driverOffset(),
      _speed(Client::MOVEMENT_SPEED) {}

void ClientVehicleType::addClassSpecificStuffToConstructionTooltip(
    std::vector<std::string>& descriptionLines) const {
  auto speedAsMultiplier = _speed / Client::MOVEMENT_SPEED;
  auto displaySpeed = proportionToPercentageString(speedAsMultiplier);
  descriptionLines.push_back("Vehicle ("s + displaySpeed + " speed)");
}

void ClientVehicleType::setImage(const std::string& imageFile) {
  _front = {imageFile + "-front.png"s, Color::MAGENTA};
  SpriteType::setImage(imageFile);
}
