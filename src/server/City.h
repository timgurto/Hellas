#ifndef CITY_H
#define CITY_H

#include <map>
#include <set>
#include <string>

#include "../Point.h"

class User;

class Kings {
 public:
  using Username = std::string;

  void add(const Username &newKing) { _container.insert(newKing); }
  bool isPlayerAKing(const Username &user) const {
    return _container.find(user) != _container.end();
  }

 private:
  std::set<Username> _container;
};

class City {
 public:
  typedef std::string Name;

  City(const Name &name = {}, const MapPoint &location = {},
       const std::string &king = {});

  typedef std::set<std::string> Members;
  const Members &members() const { return _members; }

  void update(ms_t timeElapsed);

  void addAndAlertPlayers(const User &user);
  void removeAndAlertPlayers(const User &user);
  void addPlayerWithoutAlerting(const std::string &username);
  bool isPlayerAMember(const std::string &username) const;

  void onDeclaredWar() { _hasDeclaredWar = true; }
  bool hasDeclaredWar() const { return _hasDeclaredWar; }
  ms_t timeSinceLastWarDeclaration() const {
    return _timeSinceLastWarDeclaration;
  }
  void loadTimeSinceLastWarDeclaration(ms_t newTime) {
    _timeSinceLastWarDeclaration = newTime;
  }

  const MapPoint &location() const { return _location; }
  const std::string &king() const { return _king; }

 private:
  Name _name;
  Members _members;
  MapPoint _location;  // Normally, the location of the Altar to Athena
  std::string _king;

  ms_t _timeSinceLastWarDeclaration{0};
  bool _hasDeclaredWar{false};
};

class Cities {
 public:
  void update(ms_t timeElapsed);
  void createCity(const City::Name &cityName, const MapPoint &location,
                  std::string founder);
  bool doesCityExist(const City::Name &cityName) const;

  void destroyCity(const City::Name &cityName);

  void addPlayerToCity(User &user, const City::Name &cityName);
  void removeUserFromCity(const User &user, const City::Name &cityName);
  bool isPlayerInCity(const std::string &username,
                      const City::Name &cityName) const;
  bool isPlayerInACity(const std::string &username) const;
  City::Name getPlayerCity(const std::string &username) const;
  const City::Members &membersOf(const std::string &cityName) const;
  void sendCityObjectsToCitizen(const User &citizen) const;
  MapPoint locationOf(const std::string &cityName) const;
  std::string kingOf(const std::string &cityName) const;
  void onCityDeclaredWar(std::string cityName);
  ms_t getRemainingTimeOnWarDebuff(std::string cityName,
                                   const class BuffType &buff) const;

  void sendInfoAboutCitiesTo(const User &recipient) const;

  void writeToXMLFile(const std::string &filename) const;
  void readFromXMLFile(const std::string &filename);

 private:
  std::map<City::Name, City> _container;
  std::map<std::string, City::Name> _usersToCities;

  static City::Members dummyMembersList;
};

#endif
