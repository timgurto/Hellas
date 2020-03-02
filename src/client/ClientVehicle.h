#ifndef CLIENT_VEHICLE_H
#define CLIENT_VEHICLE_H

#include "ClientObject.h"
#include "ClientVehicleType.h"

class Avatar;

class ClientVehicle : public ClientObject {
  const Avatar *_driver;

 public:
  ClientVehicle(size_t serial, const ClientVehicleType *type = nullptr,
                const MapPoint &loc = MapPoint{});
  virtual ~ClientVehicle() {}

  const ClientVehicleType &vehicleType() const {
    auto cvt = dynamic_cast<const ClientVehicleType *>(type());
    return *cvt;
  }

  const Avatar *driver() const { return _driver; }
  void driver(const Avatar *p) { _driver = p; }
  virtual double speed() const override;

  virtual char classTag() const override { return 'v'; }
  virtual void draw(const Client &client) const override;
  virtual bool addClassSpecificStuffToWindow() override;
  virtual bool addClassSpecificStuffToTooltip(Tooltip &tooltip) const override;

  static void ClientVehicle::mountOrDismount(void *object);
};

#endif
