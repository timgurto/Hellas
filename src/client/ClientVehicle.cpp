#include "ClientVehicle.h"

#include "Client.h"

extern Renderer renderer;

ClientVehicle::ClientVehicle(Client &client, Serial serial,
                             const ClientVehicleType *type, const MapPoint &loc)
    : ClientObject(serial, type, loc, client), _driver(nullptr) {}

void ClientVehicle::mountOrDismount() {
  if (!_client.character().isDriving())
    _client.sendMessage({CL_MOUNT, serial()});
  else
    _client.sendMessage({CL_DISMOUNT});
}

double ClientVehicle::speed() const { return vehicleType().speed(); }

void ClientVehicle::draw() const {
  ClientObject::draw();

  if (!driver()) return;

  // Draw driver
  const ClientVehicleType &cvt =
      dynamic_cast<const ClientVehicleType &>(*type());
  if (cvt.drawDriver()) {
    Avatar copy = *driver();
    copy.location(location() + toMapPoint(cvt.driverOffset()));
    copy.notDriving();
    copy.cutOffBottomWhenDrawn(cvt.driverCutoff());
    copy.draw();
  }

  // Draw bit in front of driver
  if (cvt.front()) {
    cvt.front().draw(drawRect() + _client.offset());
  }
}

bool ClientVehicle::addClassSpecificStuffToWindow() {
  if (isBeingConstructed() || !userHasAccess()) return false;

  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;
  Button *mountButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Enter/exit",
                 [this]() { mountOrDismount(); });
  _window->addChild(mountButton);
  y += BUTTON_GAP + BUTTON_HEIGHT;
  x += BUTTON_GAP + BUTTON_WIDTH;
  if (newWidth < x) newWidth = x;

  _window->resize(newWidth, y);

  return true;
}

bool ClientVehicle::addClassSpecificStuffToTooltip(Tooltip &tooltip) const {
  tooltip.addGap();
  auto displaySpeed = proportionToPercentageString(speed());
  tooltip.addLine("Vehicle ("s + displaySpeed + " speed)");

  return true;
}
