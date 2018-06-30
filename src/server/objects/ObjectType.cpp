#include "ObjectType.h"
#include <cassert>

ObjectType::ObjectType(const std::string &id)
    : EntityType(id),
      _numInWorld(0),
      _gatherTime(0),
      _constructionTime(1000),
      _gatherReq("none"),
      _isUnique(false),
      _isUnbuildable(false),
      _knownByDefault(false),
      _merchantSlots(0),
      _bottomlessMerchant(false),
      _container(nullptr) {
  if (_baseStats.maxHealth == 0) _baseStats.maxHealth = 1;
}

void ObjectType::addYield(const ServerItem *item, double initMean,
                          double initSD, size_t initMin, double gatherMean,
                          double gatherSD) {
  _yield.addItem(item, initMean, initSD, initMin, gatherMean, gatherSD);
}

void ObjectType::initStrengthAndMaxHealth() const {
  _baseStats.maxHealth = _strength.get();
  assert(_baseStats.maxHealth > 0);
}

void ObjectType::checkUniquenessInvariant() const {
  if (_isUnique) assert(_numInWorld <= 1);
}

void ObjectType::setHealthBasedOnItems(const ServerItem *item,
                                       size_t quantity) {
  _strength.set(item, quantity);
}

ObjectType::Strength::Strength()
    : _item(nullptr),
      _quantity(0),
      _strengthCalculated(false),
      _calculatedStrength(0) {}

void ObjectType::Strength::set(const ServerItem *item, size_t quantity) {
  _item = item;
  _quantity = quantity;
}

Hitpoints ObjectType::Strength::get() const {
  if (!_strengthCalculated) {
    if (_item == nullptr || _item->strength() == 0)
      _calculatedStrength = 1;
    else
      _calculatedStrength = _item->strength() * _quantity;
    _strengthCalculated = true;
  }
  return _calculatedStrength;
}
