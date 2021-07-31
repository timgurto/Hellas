#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <string>

#include "../messageCodes.h"
#include "City.h"
#include "EntityComponent.h"

class Entity;
class User;
class NPC;

// Manages an object's access permissions
class Permissions : public EntityComponent {
 public:
  struct Owner {
    enum Type { PLAYER, CITY, ALL_HAVE_ACCESS, NO_ACCESS, MOB };
    Owner();
    Type type;
    std::string name;

    Owner(Type type, const std::string &name);
    const std::string &typeString() const;
    bool operator<(const Owner &rhs) const;
    bool operator==(const Owner &rhs) const;
    operator bool() const { return type != ALL_HAVE_ACCESS; }
  };

  Permissions(Entity &parent) : EntityComponent(parent) {}

  void setNoAccess();
  void setOwner(const Owner &newOwner);
  void setAsMob();
  void setPlayerOwner(const std::string &username);
  void setCityOwner(const City::Name &cityName);
  bool hasOwner() const;
  bool isOwnedByPlayer(const std::string &username) const;
  bool isOwnedByCity(const City::Name &cityName) const;
  const Owner &owner() const;
  const User *getPlayerOwner() const;

  enum AccessRules { NORMAL_ACCESS, FELLOW_CITIZENS_CAN_USE_PERSONAL_OBJECTS };
  bool doesUserHaveAccess(const std::string &username,
                          AccessRules accessRules = NORMAL_ACCESS) const;
  bool doesNPCHaveAccess(const NPC &rhs) const;
  bool canUserDemolish(const std::string &username) const;

  void alertNearbyUsersToNewOwner() const;

  using Usernames = std::set<std::string>;
  Usernames ownerAsUsernames();

 private:
  Owner _owner;
};

#endif
