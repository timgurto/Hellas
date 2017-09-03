#ifndef CITY_H
#define CITY_H

#include <map>
#include <set>
#include <string>

class User;


class Kings {
public:
    using Username = std::string;

    void add(const Username &newKing) { _container.insert(newKing); }
    bool isPlayerAKing(const Username &user) const { return _container.find(user) != _container.end(); }

private:
    std::set<Username> _container;
};


class City{
public:
    typedef std::string Name;

    City(const Name &name);

    typedef std::set<std::string> Members;
    const Members &members() const { return _members; }

    void addAndAlertPlayers(const User &user);
    void removeAndAlertPlayers(const User &user);
    void addPlayerWithoutAlerting(const std::string &username);
    bool isPlayerAMember(const std::string &username) const;

private:
    Name _name;
    Members _members;
};


class Cities{
public:
    void createCity(const City::Name &cityName);
    bool doesCityExist(const City::Name &cityName) const;

    void destroyCity(const City::Name &cityName);

    void addPlayerToCity(const User &user, const City::Name &cityName);
    void removeUserFromCity(const User &user, const City::Name &cityName);
    bool isPlayerInCity(const std::string &username, const City::Name &cityName) const;
    const City::Name &getPlayerCity(const std::string &username) const;
    const City::Members &membersOf(const std::string &cityName) const;
    
    void writeToXMLFile(const std::string &filename) const;
    void readFromXMLFile(const std::string &filename);

private:
    std::map<City::Name, City> _container;
    std::map<std::string, City::Name> _usersToCities;
};

#endif
