#include <cassert>

#include "City.h"
#include "Server.h"
#include "../XmlReader.h"
#include "../XmlWriter.h"

City::City(const Name &name = ""):
_name(name)
{}

void City::addAndAlertPlayer(const User &user) {
    _members.insert(user.name());
    Server::instance().sendMessage(user.socket(), SV_JOINED_CITY, _name);
}

void City::removeAndAlertPlayer(const User &user) {
    _members.erase(user.name());
}

void City::addPlayerWithoutAlerting(const std::string &username){
    _members.insert(username);
}

bool City::isPlayerAMember(const std::string &username) const{
    return _members.find(username) != _members.end();
}




void Cities::createCity(const City::Name &cityName){
    if (doesCityExist(cityName)){
        Server::debug()("Can't create city: a city with that name already exists", Color::FAILURE);
        return;
    }
    _container[cityName] = City(cityName);
}

bool Cities::doesCityExist(const City::Name &cityName) const {
    return _container.find(cityName) != _container.end();
}

void Cities::addPlayerToCity(const User &user, const City::Name &cityName){
    auto it = _container.find(cityName);
    bool cityExists = it != _container.end();
    assert(cityExists);
    City &city = it->second;
    city.addAndAlertPlayer(user);
    _usersToCities[user.name()] = cityName;
}

void Cities::removeUserFromCity(const User &user, const City::Name &cityName) {
    auto it = _container.find(cityName);
    bool cityExists = it != _container.end();
    assert(cityExists);
    City &city = it->second;
    city.removeAndAlertPlayer(user);
}

bool Cities::isPlayerInCity(const std::string &username, const City::Name &cityName) const{
    auto it = _container.find(cityName);
    bool cityExists = it != _container.end();
    if (! cityExists){
        Server::debug()("Can't check membership for a city that doesn't exist", Color::FAILURE);
        return false;
    }
    const City &city = it->second;
    return city.isPlayerAMember(username);
}

const City::Name &Cities::getPlayerCity(const std::string &username) const{
    auto it = _usersToCities.find(username);
    static const std::string BLANK = "";
    if (it == _usersToCities.end())
        return BLANK;
    return it->second;

}

void Cities::writeToXMLFile(const std::string &filename) const{
    XmlWriter xw(filename);
    for (const auto &pair : _container){

        auto e = xw.addChild("city");
        xw.setAttr(e, "name", pair.first);

        const City &city = pair.second;
        for (const std::string &member : city.members()){
            auto memberE = xw.addChild("member", e);
            xw.setAttr(memberE, "username", member);
        }
    }
    xw.publish();
}

void Cities::readFromXMLFile(const std::string &filename){
    XmlReader xr(filename);
    if (! xr){
        Server::debug()("Failed to load data from " + filename, Color::FAILURE);
        return;
    }
    for (auto elem : xr.getChildren("city")){
        City::Name name;
        if (!xr.findAttr(elem, "name", name))
            continue;
        createCity(name);
        City &city = _container[name];
        for (auto memberElem : xr.getChildren("member", elem)){
            std::string username;
            if (!xr.findAttr(memberElem, "username", username))
                continue;
            city.addPlayerWithoutAlerting(username);
            _usersToCities[username] = name;
        }
    }
}

const City::Members &Cities::membersOf(const std::string &cityName) const{
    auto it = _container.find(cityName);
    bool cityExists = it != _container.end();
    if (! cityExists){
        Server::debug()("Can't fetch members of a city that doesn't exist", Color::FAILURE);
        assert(false);
    }
    return it->second.members();
}
