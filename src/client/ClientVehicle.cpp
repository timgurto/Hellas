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

  // Draw driver
  const ClientVehicleType &cvt =
      dynamic_cast<const ClientVehicleType &>(*type());
  if (cvt.drawDriver() && driver() != nullptr) {
    Avatar copy = *driver();
    copy.location(location() + toMapPoint(cvt.driverOffset()));
    copy.driving(false);
    copy.draw(client);
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
