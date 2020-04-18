#include "City.h"

#include "../XmlReader.h"
#include "../XmlWriter.h"
#include "Server.h"

City::Members Cities::dummyMembersList{};

City::City(const Name &name, const MapPoint &location)
    : _name(name), _location(location) {}

void City::addAndAlertPlayers(const User &user) {
  _members.insert(user.name());
  user.sendMessage({SV_JOINED_CITY, _name});
  Server::instance().broadcastToArea(
      user.location(), {SV_IN_CITY, makeArgs(user.name(), _name)});
}

void City::removeAndAlertPlayers(const User &user) {
  _members.erase(user.name());
  Server::instance().broadcastToArea(user.location(),
                                     {SV_NO_CITY, user.name()});
}

void City::addPlayerWithoutAlerting(const std::string &username) {
  _members.insert(username);
}

bool City::isPlayerAMember(const std::string &username) const {
  return _members.count(username) == 1;
}

void Cities::createCity(const City::Name &cityName, const MapPoint &location) {
  if (doesCityExist(cityName)) {
    Server::debug()("Can't create city: a city with that name already exists",
                    Color::CHAT_ERROR);
    return;
  }
  _container[cityName] = {cityName, location};
}

bool Cities::doesCityExist(const City::Name &cityName) const {
  return _container.find(cityName) != _container.end();
}

void Cities::destroyCity(const City::Name &cityName) {
  // Kill all city objects
  auto city = Permissions::Owner(Permissions::Owner::CITY, cityName);
  Server::instance().killAllObjectsOwnedBy(city);

  // Remove all citizens
  auto citizenNames = membersOf(cityName);
  for (const auto &citizenName : citizenNames) {
    const auto *citizen = Server::instance().getUserByName(citizenName);
    if (citizen == nullptr) continue;
    removeUserFromCity(*citizen, cityName);
  }

  // Remove city
}

void Cities::addPlayerToCity(const User &user, const City::Name &cityName) {
  auto it = _container.find(cityName);
  bool cityExists = it != _container.end();
  if (!cityExists) {
    SERVER_ERROR("Tried to add player to a non-existent city");
    return;
  }
  City &city = it->second;
  city.addAndAlertPlayers(user);
  _usersToCities[user.name()] = cityName;
  sendCityObjectsToCitizen(user);
}

void Cities::removeUserFromCity(const User &user, const City::Name &cityName) {
  auto it = _container.find(cityName);
  bool cityExists = it != _container.end();
  if (!cityExists) {
    SERVER_ERROR("Tried to add player to a non-existent city");
    return;
  }
  City &city = it->second;
  city.removeAndAlertPlayers(user);
  _usersToCities.erase(user.name());
}

bool Cities::isPlayerInCity(const std::string &username,
                            const City::Name &cityName) const {
  auto it = _container.find(cityName);
  bool cityExists = it != _container.end();
  if (!cityExists) {
    Server::debug()("Can't check membership for a city that doesn't exist",
                    Color::CHAT_ERROR);
    return false;
  }
  const City &city = it->second;
  return city.isPlayerAMember(username);
}

bool Cities::isPlayerInACity(const std::string &username) const {
  auto it = _usersToCities.find(username);
  return it != _usersToCities.end();
}

City::Name Cities::getPlayerCity(const std::string &username) const {
  auto it = _usersToCities.find(username);
  if (it == _usersToCities.end()) return {};
  return it->second;
}

void Cities::writeToXMLFile(const std::string &filename) const {
  XmlWriter xw(filename);
  for (const auto &pair : _container) {
    const City &city = pair.second;

    auto e = xw.addChild("city");
    xw.setAttr(e, "name", pair.first);

    auto locE = xw.addChild("location", e);
    xw.setAttr(locE, "x", city.location().x);
    xw.setAttr(locE, "y", city.location().y);

    for (const std::string &member : city.members()) {
      auto memberE = xw.addChild("member", e);
      xw.setAttr(memberE, "username", member);
    }
  }
  xw.publish();
}

void Cities::readFromXMLFile(const std::string &filename) {
  auto xr = XmlReader::FromFile(filename);
  if (!xr) {
    Server::debug()("Failed to load data from " + filename, Color::CHAT_ERROR);
    return;
  }
  for (auto elem : xr.getChildren("city")) {
    City::Name name;
    if (!xr.findAttr(elem, "name", name)) continue;

    auto locationElem = xr.findChild("location", elem);
    if (!locationElem) continue;
    auto location = MapPoint{};
    xr.findAttr(locationElem, "x", location.x);
    xr.findAttr(locationElem, "y", location.y);

    createCity(name, location);
    City &city = _container[name];
    for (auto memberElem : xr.getChildren("member", elem)) {
      std::string username;
      if (!xr.findAttr(memberElem, "username", username)) continue;
      city.addPlayerWithoutAlerting(username);
      _usersToCities[username] = name;
    }
  }
}

const City::Members &Cities::membersOf(const std::string &cityName) const {
  auto it = _container.find(cityName);
  bool cityExists = it != _container.end();
  if (!cityExists) {
    SERVER_ERROR("Can't fetch members of a city that doesn't exist");
    return dummyMembersList;
  }
  return it->second.members();
}

void Cities::sendCityObjectsToCitizen(const User &citizen) const {
  const auto cityName = getPlayerCity(citizen.name());
  if (cityName.empty()) return;

  auto iterators = Server::instance().findObjectsOwnedBy(
      {Permissions::Owner::CITY, cityName});
  for (auto it = iterators.first; it != iterators.second; ++it) {
    auto serial = *it;
    auto *entity = Server::instance().findEntityBySerial(serial);
    entity->sendInfoToClient(citizen);
  }
}

MapPoint Cities::locationOf(const std::string &cityName) const {
  auto it = _container.find(cityName);
  bool cityExists = it != _container.end();
  if (!cityExists) {
    SERVER_ERROR("Can't get location of a city that doesn't exist");
    return {};
  }
  return it->second.location();
}

void Cities::sendInfoAboutCitiesTo(const User &recipient) const {
  for (const auto &city : _container) recipient.sendMessage({SV_CITY_DETAILS});
}
