#ifndef OBJECTS_BY_OWNER_H
#define OBJECTS_BY_OWNER_H

#include <set>

#include "../Serial.h"
#include "Permissions.h"

class Object;

class ObjectsByOwner {
  class ObjectsWithSpecificOwner;

 public:
  bool isObjectOwnedBy(Serial serial, const Permissions::Owner &owner) const;
  const ObjectsWithSpecificOwner &getObjectsWithSpecificOwner(
      const Permissions::Owner &owner) const;
  void add(const Permissions::Owner &owner, Serial serial) {
    container[owner].add(serial);
  }
  void remove(const Permissions::Owner &owner, Serial serial) {
    container[owner].remove(serial);
  }

  std::pair<std::set<Serial>::iterator, std::set<Serial>::iterator>
  getObjectsOwnedBy(const Permissions::Owner &owner) const;

 private:
  class ObjectsWithSpecificOwner {
   public:
    size_t size() const { return container.size(); }
    void add(Serial serial);
    void remove(Serial serial);
    bool isObjectOwned(Serial serial) const;

    using Container = std::set<Serial>;
    Container::const_iterator begin() const { return container.begin(); }
    Container::const_iterator end() const { return container.end(); }

   private:
    Container container;
  };

  std::map<Permissions::Owner, ObjectsWithSpecificOwner> container;
  static ObjectsWithSpecificOwner EmptyQueryResult;
};

#endif
