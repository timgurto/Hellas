#include <map>

#include "Permissions.h"
#include "Server.h"
#include "User.h"

Permissions::Owner::Owner() : type(NONE) {}

Permissions::Owner::Owner(Type typeArg, const std::string &nameArg)
    : type(typeArg), name(nameArg) {}

const std::string &Permissions::Owner::typeString() const {
  static std::map<Type, std::string> typeStrings;
  if (typeStrings.empty()) {
    typeStrings[PLAYER] = "player";
    typeStrings[CITY] = "city";
  }
  return typeStrings[type];
}

bool Permissions::Owner::operator<(const Owner &rhs) const {
  if (type != rhs.type) return type < rhs.type;
  return name < rhs.name;
}

void Permissions::setPlayerOwner(const std::string &username) {
  ObjectsByOwner &ownerIndex = Server::_instance->_objectsByOwner;
  ownerIndex.remove(_owner, _parent.serial());

  _owner.type = Owner::PLAYER;
  _owner.name = username;

  ownerIndex.add(_owner, _parent.serial());

  alertNearbyUsersToNewOwner();
}

void Permissions::setCityOwner(const City::Name &cityName) {
  ObjectsByOwner &ownerIndex = Server::_instance->_objectsByOwner;
  ownerIndex.remove(_owner, _parent.serial());

  const Cities &cities = Server::instance()._cities;
  if (!cities.doesCityExist(cityName)) return;
  _owner.type = Owner::CITY;
  _owner.name = cityName;

  ownerIndex.add(_owner, _parent.serial());

  alertNearbyUsersToNewOwner();
}

bool Permissions::hasOwner() const { return _owner.type != Owner::NONE; }

bool Permissions::isOwnedByPlayer(const std::string &username) const {
  return _owner.type == Owner::PLAYER && _owner.name == username;
}

bool Permissions::isOwnedByCity(const City::Name &cityName) const {
  return _owner.type == Owner::CITY && _owner.name == cityName;
}

const Permissions::Owner &Permissions::owner() const { return _owner; }

bool Permissions::doesUserHaveAccess(const std::string &username) const {
  switch (_owner.type) {
    case Owner::NONE:
      return true;

    case Owner::PLAYER:
      return _owner.name == username;

    case Owner::CITY: {
      const Cities &cities = Server::instance()._cities;
      const std::string &playerCity = cities.getPlayerCity(username);
      return playerCity == _owner.name;
    }

    default:
      Server::error("Checking permission when owner has no type");
      return false;
  }
}

void Permissions::alertNearbyUsersToNewOwner() const {
  auto &server = *Server::_instance;
  server.broadcastToArea(
      _parent.location(), SV_OWNER,
      makeArgs(_parent.serial(), _owner.typeString(), _owner.name));
}

Permissions::Usernames Permissions::ownerAsUsernames() {
  const Server &server = Server::instance();
  switch (_owner.type) {
    case Owner::NONE:
      return {};
    case Owner::PLAYER:
      return {_owner.name};
    case Owner::CITY:
      return server.cities().membersOf(_owner.name);
    default:
      Server::error("Trying to fetch owning users when owner has no type");
      return {};
  }
}
