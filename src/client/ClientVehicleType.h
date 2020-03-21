#ifndef CLIENT_VEHICLE_TYPE_H
#define CLIENT_VEHICLE_TYPE_H

#include "ClientObjectType.h"

class ClientVehicleType : public ClientObjectType {
  bool _drawDriver{false};
  ScreenPoint _driverOffset;      // Where to draw the driver
  px_t _cutOffBottomOfDriver{0};  // Don't draw the bottom pixels of the driver
  double _speed{1.0};
  Texture _front;

 public:
  ClientVehicleType(const std::string &id);
  virtual char classTag() const override { return 'v'; }

  void setSpeed(double s) { _speed = s; }
  double speed() const { return _speed; }

  bool drawDriver() const { return _drawDriver; }
  void drawDriver(bool b) { _drawDriver = b; }
  const ScreenPoint &driverOffset() const { return _driverOffset; }
  void driverOffset(const ScreenPoint &p) { _driverOffset = p; }
  px_t driverCutoff() const { return _cutOffBottomOfDriver; }
  void cutOffBottomOfDriver(px_t bottomRows) {
    _cutOffBottomOfDriver = bottomRows;
  }
  const Texture &front() const { return _front; }

  virtual void addClassSpecificStuffToConstructionTooltip(
      std::vector<std::string> &descriptionLines) const override;
  virtual void setImage(const std::string &imageFile) override;
};

#endif
