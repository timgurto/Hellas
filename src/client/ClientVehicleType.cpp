#include "ClientVehicleType.h"

#include "Client.h"

ClientVehicleType::ClientVehicleType(const std::string& id)
    : ClientObjectType(id) {}

void ClientVehicleType::addClassSpecificStuffToConstructionTooltip(
    std::vector<std::string>& descriptionLines) const {
  auto displaySpeed = proportionToPercentageString(_speed);
  descriptionLines.push_back("Vehicle ("s + displaySpeed + " speed)");
}

void ClientVehicleType::setImage(const std::string& imageFile) {
  _front = {imageFile + "-front.png"s, Color::MAGENTA};
  _front.dontDrawPlaceholderIfInvalid();
  SpriteType::setImage(imageFile);
}
