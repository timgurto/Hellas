#ifndef VEHICLE_H
#define VEHICLE_H

#include "objects/Object.h"

class VehicleType;

class Vehicle : public Object {
  std::string _driver{};

 public:
  Vehicle(const VehicleType *type, const MapPoint &loc);
  virtual ~Vehicle() {}

  const std::string &driver() const { return _driver; }
  void driver(const std::string &username) { _driver = username; }
  bool shouldMoveWhereverRequested() const override;

  char classTag() const override { return 'v'; }
};

#endif
