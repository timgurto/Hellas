#include "ObjectsByOwner.h"

ObjectsByOwner::ObjectsWithSpecificOwner ObjectsByOwner::EmptyQueryResult;

const ObjectsByOwner::ObjectsWithSpecificOwner &
ObjectsByOwner::getObjectsWithSpecificOwner(
    const Permissions::Owner &owner) const {
  auto it = container.find(owner);
  if (it == container.end()) return EmptyQueryResult;
  return it->second;
}

std::pair<std::set<Serial>::iterator, std::set<Serial>::iterator>
ObjectsByOwner::getObjectsOwnedBy(const Permissions::Owner &owner) const {
  const auto &objects = getObjectsWithSpecificOwner(owner);
  return std::make_pair(objects.begin(), objects.end());
}

bool ObjectsByOwner::isObjectOwnedBy(Serial serial,
                                     const Permissions::Owner &owner) const {
  const auto &hisObjects = getObjectsWithSpecificOwner(owner);
  return hisObjects.isObjectOwned(serial);
}

void ObjectsByOwner::ObjectsWithSpecificOwner::add(Serial serial) {
  container.insert(serial);
}

void ObjectsByOwner::ObjectsWithSpecificOwner::remove(Serial serial) {
  container.erase(serial);
}

bool ObjectsByOwner::ObjectsWithSpecificOwner::isObjectOwned(
    Serial serial) const {
  auto it = container.find(serial);
  return it != container.end();
}
