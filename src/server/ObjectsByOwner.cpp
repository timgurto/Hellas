#include "ObjectsByOwner.h"

ObjectsByOwner::ObjectsWithSpecificOwner ObjectsByOwner::EmptyQueryResult;

const ObjectsByOwner::ObjectsWithSpecificOwner &
ObjectsByOwner::getObjectsWithSpecificOwner(
    const Permissions::Owner &owner) const {
  auto it = container.find(owner);
  if (it == container.end()) return EmptyQueryResult;
  return it->second;
}

bool ObjectsByOwner::isObjectOwnedBy(size_t serial,
                                     const Permissions::Owner &owner) const {
  const auto &hisObjects = getObjectsWithSpecificOwner(owner);
  return hisObjects.isObjectOwned(serial);
}

void ObjectsByOwner::ObjectsWithSpecificOwner::add(size_t serial) {
  container.insert(serial);
}

void ObjectsByOwner::ObjectsWithSpecificOwner::remove(size_t serial) {
  container.erase(serial);
}

bool ObjectsByOwner::ObjectsWithSpecificOwner::isObjectOwned(
    size_t serial) const {
  auto it = container.find(serial);
  return it != container.end();
}
