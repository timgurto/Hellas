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

void Permissions::setPlayerOwner(const std::string &username) {
  ObjectsByOwner &ownerIndex = Server::_instance->_objectsByOwner;
  ownerIndex.remove(_owner, parent().serial());

  _owner.type = Owner::PLAYER;
  _owner.name = username;

  ownerIndex.add(_owner, parent().serial());

  alertNearbyUsersToNewOwner();
  parent().onOwnershipChange();
}

void Permissions::setCityOwner(const City::Name &cityName) {
  ObjectsByOwner &ownerIndex = Server::_instance->_objectsByOwner;
  ownerIndex.remove(_owner, parent().serial());

  const Cities &cities = Server::instance()._cities;
  if (!cities.doesCityExist(cityName)) return;
  _owner.type = Owner::CITY;
  _owner.name = cityName;

  ownerIndex.add(_owner, parent().serial());

  alertNearbyUsersToNewOwner();
  parent().onOwnershipChange();
}

bool Permissions::hasOwner() const {
  return _owner.type != Owner::ALL_HAVE_ACCESS;
}

bool Permissions::isOwnedByPlayer(const std::string &username) const {
  return _owner.type == Owner::PLAYER && _owner.name == username;
}

bool Permissions::isOwnedByCity(const City::Name &cityName) const {
  return _owner.type == Owner::CITY && _owner.name == cityName;
}

const Permissions::Owner &Permissions::owner() const { return _owner; }

bool Permissions::doesUserHaveAccess(const std::string &username,
                                     bool allowFellowCitizens) const {
  // Unowned
  if (_owner.type == Owner::ALL_HAVE_ACCESS) return true;
  if (_owner.type == Owner::NO_ACCESS) return false;

  // Owned by player
  if (_owner == Owner{Owner::PLAYER, username}) return true;

  const Cities &cities = Server::instance()._cities;
  const std::string &playerCity = cities.getPlayerCity(username);
  if (playerCity.empty()) return false;

  // Owned by player's city
  if (_owner == Owner{Owner::CITY, playerCity}) return true;

  // Fellow citizens (e.g., for altars)
  if (!allowFellowCitizens) return false;
  if (_owner.type != Owner::PLAYER) return false;
  auto ownerCity = cities.getPlayerCity(_owner.name);
  if (ownerCity.empty()) return false;
  return playerCity == ownerCity;
}

bool Permissions::canUserDemolish(const std::string &username) const {
  // Requires ownership; more strict than doesUserHaveAccess().

  // Owned by player
  if (_owner == Owner{Owner::PLAYER, username}) return true;

  return false;
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
