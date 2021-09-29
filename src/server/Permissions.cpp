#include "Permissions.h"

#include <map>

#include "Server.h"
#include "User.h"

Permissions::Owner::Owner() : type(ALL_HAVE_ACCESS) {}

Permissions::Owner::Owner(Type typeArg, const std::string &nameArg)
    : type(typeArg), name(nameArg) {}

const std::string &Permissions::Owner::typeString() const {
  static std::map<Type, std::string> typeStrings;
  if (typeStrings.empty()) {
    typeStrings[PLAYER] = "player";
    typeStrings[CITY] = "city";
    typeStrings[NO_ACCESS] = "noAccess";
  }
  return typeStrings[type];
}

bool Permissions::Owner::operator<(const Permissions::Owner &rhs) const {
  if (type != rhs.type) return type < rhs.type;
  return name < rhs.name;
}

bool Permissions::Owner::operator==(const Permissions::Owner &rhs) const {
  return type == rhs.type && name == rhs.name;
}

void Permissions::setNoAccess() {
  ObjectsByOwner &ownerIndex = Server::_instance->_objectsByOwner;
  ownerIndex.remove(_owner, parent().serial());

  _owner.type = Owner::NO_ACCESS;
  _owner.name = {};

  ownerIndex.add(_owner, parent().serial());

  alertNearbyUsersToNewOwner();
  parent().onOwnershipChange();
}

void Permissions::setOwner(const Owner &newOwner) {
  ObjectsByOwner &ownerIndex = Server::_instance->_objectsByOwner;
  ownerIndex.remove(_owner, parent().serial());

  _owner = newOwner;

  ownerIndex.add(_owner, parent().serial());

  alertNearbyUsersToNewOwner();
  parent().onOwnershipChange();
}

void Permissions::setAsMob() { _owner.type = Owner::MOB; }

void Permissions::setPlayerOwner(const std::string &username) {
  setOwner({Owner::PLAYER, username});

  const auto *user = Server::instance().getUserByName(username);
  if (user) parent().sendInfoToClient(*user);
}

void Permissions::setCityOwner(const City::Name &cityName) {
  auto &server = Server::instance();
  if (!server.cities().doesCityExist(cityName)) return;

  setOwner({Owner::CITY, cityName});
}

bool Permissions::hasOwner() const {
  if (_owner.type == Owner::ALL_HAVE_ACCESS) return false;
  if (_owner.type == Owner::MOB) return false;
  return true;
}

bool Permissions::isOwnedByPlayer(const std::string &username) const {
  return _owner.type == Owner::PLAYER && _owner.name == username;
}

bool Permissions::isOwnedByCity(const City::Name &cityName) const {
  return _owner.type == Owner::CITY && _owner.name == cityName;
}

bool Permissions::isOwnedByPlayerCity(std::string username) const {
  const auto &cities = Server::instance()._cities;
  const auto playerCity = cities.getPlayerCity(username);
  if (playerCity.empty()) return false;
  return isOwnedByCity(playerCity);
}

const Permissions::Owner &Permissions::owner() const { return _owner; }

const User *Permissions::getPlayerOwner() const {
  if (_owner.type != Owner::PLAYER) return nullptr;

  auto &server = Server::instance();
  return server.getUserByName(_owner.name);
}

bool Permissions::doesUserHaveNormalAccess(const std::string &username) const {
  // Unowned
  if (_owner.type == Owner::ALL_HAVE_ACCESS) return true;
  if (_owner.type == Owner::NO_ACCESS) return false;

  return isOwnedByPlayer(username) || isOwnedByPlayerCity(username);
}

bool Permissions::canUserGiveAway(std::string username) const {
  if (_owner.type == Owner::ALL_HAVE_ACCESS) return false;  // More strict
  if (_owner.type == Owner::NO_ACCESS) return false;

  if (isOwnedByPlayer(username)) return true;

  // Must be king to give away a city object
  const auto playerIsKing = Server::instance()._kings.isPlayerAKing(username);
  return isOwnedByPlayerCity(username) && playerIsKing;
}

bool Permissions::canUserPerformAction(std::string username) const {
  // Unowned
  if (_owner.type == Owner::ALL_HAVE_ACCESS) return true;
  if (_owner.type == Owner::NO_ACCESS) return false;

  if (isOwnedByPlayer(username)) return true;
  if (isOwnedByPlayerCity(username)) return true;

  if (_owner.type != Owner::PLAYER) return false;  // Some other city

  // Owned by a fellow citizen (e.g., for altars)
  const auto &cities = Server::instance()._cities;
  const auto ownerCity = cities.getPlayerCity(_owner.name);
  if (ownerCity.empty()) return false;
  const auto playerCity = cities.getPlayerCity(username);
  return playerCity == ownerCity;
}

bool Permissions::canUserAccessContainer(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canUserLoot(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canUserDemolish(std::string username) const {
  if (_owner.type == Owner::ALL_HAVE_ACCESS) return false;  // More strict
  if (_owner.type == Owner::NO_ACCESS) return false;

  return isOwnedByPlayer(username) || isOwnedByPlayerCity(username);
}

bool Permissions::canUserRepair(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canUserPickUp(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canUserSetMerchantSlots(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canUserMount(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canUserOverlap(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canUserUseAsTool(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canUserGather(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canUserGetBuffs(std::string username) const {
  return doesUserHaveNormalAccess(username);
}

bool Permissions::canNPCOverlap(const NPC &rhs) const {
  if (!rhs.permissions.hasOwner()) return false;
  // Assume a player, not a city, as pets can't be owned by cities
  const auto &ownerPlayerName = rhs.permissions.owner().name;
  return canUserOverlap(ownerPlayerName);
}

void Permissions::alertNearbyUsersToNewOwner() const {
  auto &server = *Server::_instance;
  server.broadcastToArea(
      parent().location(),
      {SV_OWNER,
       makeArgs(parent().serial(), _owner.typeString(), _owner.name)});
}

Permissions::Usernames Permissions::ownerAsUsernames() {
  const Server &server = Server::instance();
  switch (_owner.type) {
    case Owner::ALL_HAVE_ACCESS:
    case Owner::NO_ACCESS:
      return {};
    case Owner::PLAYER:
      return {_owner.name};
    case Owner::CITY:
      return server.cities().membersOf(_owner.name);
    default:
      SERVER_ERROR("Trying to fetch owning users when owner has no type");
      return {};
  }
}
