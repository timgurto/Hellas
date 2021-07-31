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

 private:
  bool doesUserHaveNormalAccess(const std::string &username) const;

 public:
  bool canUserGiveAway(std::string username) const;
  bool canUserPerformAction(std::string username) const;
  bool canUserAccessContainer(std::string username) const;
  bool canUserLoot(std::string username) const;
  bool canUserDemolish(std::string username) const;
  bool canUserRepair(std::string username) const;
  bool canUserPickUp(std::string username) const;
  bool canUserSetMerchantSlots(std::string username) const;
  bool canUserMount(std::string username) const;
  bool canUserOverlap(std::string username) const;
  bool canUserUseAsTool(std::string username) const;
  bool canUserGather(std::string username) const;
  bool canUserGetBuffs(std::string username) const;
  bool canNPCOverlap(const NPC &rhs) const;

  void alertNearbyUsersToNewOwner() const;

  using Usernames = std::set<std::string>;
  Usernames ownerAsUsernames();

 private:
  Owner _owner;
};

#endif
