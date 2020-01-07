#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <string>

#include "../messageCodes.h"
#include "City.h"

class Entity;
class User;

// Manages an object's access permissions
class Permissions {
 public:
  struct Owner {
    enum Type { PLAYER, CITY, ALL_HAVE_ACCESS, NO_ACCESS };
    Owner();
    Type type;
    std::string name;

    Owner(Type type, const std::string &name);
    const std::string &typeString() const;
    bool operator<(const Owner &rhs) const;
    bool operator==(const Owner &rhs) const;
    operator bool() const { return type != ALL_HAVE_ACCESS; }
  };

  Permissions(Entity &parent) : _parent(parent) {}

  void setNoAccess();
  void setPlayerOwner(const std::string &username);
  void setCityOwner(const City::Name &cityName);
  bool hasOwner() const;
  bool isOwnedByPlayer(const std::string &username) const;
  bool isOwnedByCity(const City::Name &cityName) const;
  const Owner &owner() const;

  bool doesUserHaveAccess(const std::string &username,
                          bool allowFellowCitizens = false) const;
  bool canUserDemolish(const std::string &username) const;

  void alertNearbyUsersToNewOwner() const;

  using Usernames = std::set<std::string>;
  Usernames ownerAsUsernames();

 private:
  Owner _owner;
  Entity &_parent;
};

#endif
