#include "Durability.h"

#include "ServerItem.h"

Strength::Strength()
    : _item(nullptr),
      _quantity(0),
      _strengthCalculated(false),
      _calculatedStrength(0) {}

void Strength::set(const ServerItem *item, size_t quantity) {
  _item = item;
  _quantity = quantity;
}

Hitpoints Strength::get() const {
  if (!_strengthCalculated) {
    if (_item == nullptr || _item->strength() == 0)
      _calculatedStrength = 1;
    else
      _calculatedStrength = _item->strength() * _quantity;
    _strengthCalculated = true;
  }
  return _calculatedStrength;
}
