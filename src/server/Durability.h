#pragma once

#include "../types.h"

class ServerItem;

class Strength {
 public:
  Strength();
  void set(const ServerItem *item, size_t quantity);
  Hitpoints get() const;
  const ServerItem *item() const { return _item; }
  size_t quantity() const { return _quantity; }

 private:
  const ServerItem *_item;
  size_t _quantity;
  mutable Hitpoints _calculatedStrength;
  mutable bool _strengthCalculated;
};
