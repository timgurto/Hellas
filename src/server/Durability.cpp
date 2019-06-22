#include "Durability.h"

#include "ServerItem.h"

Durability::Durability()
    : _item(nullptr),
      _quantity(0),
      _strengthCalculated(false),
      _calculatedStrength(0) {}

void Durability::set(const ServerItem *item, size_t quantity) {
  _item = item;
  _quantity = quantity;
}

Hitpoints Durability::get() const {
  if (!_strengthCalculated) {
    if (_item == nullptr || _item->durability() == 0)
      _calculatedStrength = 1;
    else
      _calculatedStrength = _item->durability() * _quantity;
    _strengthCalculated = true;
  }
  return _calculatedStrength;
}
