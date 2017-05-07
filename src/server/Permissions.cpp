#include <cassert>
#include <map>

#include "Permissions.h"
#include "Server.h"
#include "User.h"

Permissions::Owner::Owner():
type(NONE){}

const std::string &Permissions::Owner::typeString() const{
    static std::map<Type, std::string> typeStrings;
    if (typeStrings.empty()){
        typeStrings[PLAYER] = "player";
        typeStrings[CITY] = "city";
    }
    return typeStrings[type];
}

void Permissions::setPlayerOwner(const std::string &username) {
    _owner.type = Owner::PLAYER;
    _owner.name = username;
}

void Permissions::setCityOwner(const City::Name &cityName) {
    const Cities &cities = Server::instance()._cities;
    if (! cities.doesCityExist(cityName))
        return;
    _owner.type = Owner::CITY;
    _owner.name = cityName;
}

bool Permissions::hasOwner() const{
    return _owner.type != Owner::NONE;
}

bool Permissions::isOwnedByPlayer(const std::string &username) const{
    return _owner.type == Owner::PLAYER &&
           _owner.name == username;
}

bool Permissions::isOwnedByCity(const City::Name &cityName) const{
    return _owner.type == Owner::CITY &&
           _owner.name == cityName;

}

const Permissions::Owner &Permissions::owner() const {
    return _owner;
}

bool Permissions::doesUserHaveAccess(const std::string &username) const{
    switch (_owner.type){
    case Owner::NONE:
        return true;

    case Owner::PLAYER:
        return _owner.name == username;

    case Owner::CITY:
    {
        const Cities &cities = Server::instance()._cities;
        const std::string &playerCity = cities.getPlayerCity(username);
        return playerCity == _owner.name;
    }

    default:
        assert(false);
        return false;
    }

}
