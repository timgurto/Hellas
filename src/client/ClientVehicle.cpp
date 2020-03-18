#include "ClientVehicle.h"

#include "Client.h"

ClientVehicle::ClientVehicle(size_t serial, const ClientVehicleType *type,
                             const MapPoint &loc)
    : ClientObject(serial, type, loc), _driver(nullptr) {}

void ClientVehicle::mountOrDismount(void *object) {
  const ClientVehicle &obj = *static_cast<const ClientVehicle *>(object);
  Client &client = *Client::_instance;

  // Not currently driving anything: attempt to mount
  if (!client.character().isDriving())
    client.sendMessage({CL_MOUNT, obj.serial()});

  // Currently driving: attempt to dismount
  else
    client.attemptDismount();
}

double ClientVehicle::speed() const { return vehicleType().speed(); }

void ClientVehicle::draw(const Client &client) const {
  ClientObject::draw(client);

  if (!driver()) return;

  // Draw driver
  const ClientVehicleType &cvt =
      dynamic_cast<const ClientVehicleType &>(*type());
  if (cvt.drawDriver()) {
    Avatar copy = *driver();
    copy.location(location() + toMapPoint(cvt.driverOffset()));
    copy.notDriving();
    copy.draw(client);
  }

  // Draw bit in front of driver
  if (cvt.front()) {
    cvt.front().draw(drawRect() + client.offset());
  }
}

bool ClientVehicle::addClassSpecificStuffToWindow() {
  if (isBeingConstructed() || !userHasAccess()) return false;

  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;
  Button *mountButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Enter/exit",
                 [this]() { ClientVehicle::mountOrDismount(this); });
  _window->addChild(mountButton);
  y += BUTTON_GAP + BUTTON_HEIGHT;
  x += BUTTON_GAP + BUTTON_WIDTH;
  if (newWidth < x) newWidth = x;

  _window->resize(newWidth, y);

  return true;
}

bool ClientVehicle::addClassSpecificStuffToTooltip(Tooltip &tooltip) const {
  auto speedAsMultiplier = speed() / Client::MOVEMENT_SPEED;
  auto displaySpeed = proportionToPercentageString(speedAsMultiplier);
  tooltip.addLine("Vehicle ("s + displaySpeed + " speed)");

  return true;
}
