#include "Permissions.h"
#include "User.h"

Permissions::Owner::Owner():
type(NONE){}

void Permissions::setPlayerOwner(const std::string &username) {
    _owner.type = PLAYER;
    _owner.name = username;
}

void Permissions::setCityOwner(const City::Name &cityName) {
    _owner.type = CITY;
    _owner.name = cityName;
}

bool Permissions::isOwnedByPlayer(const std::string &username) const{
    return _owner.type == PLAYER &&
           _owner.name == username;
}

bool Permissions::isOwnedByCity(const City::Name &cityName) const{
    return _owner.type == CITY &&
           _owner.name == cityName;

}

const Permissions::Owner &Permissions::owner() const {
    return _owner;
}

bool Permissions::doesUserHaveAccess(const std::string &username) const{
    switch (_owner.type){
    case NONE:
        return true;

    case PLAYER:
        return _owner.name == username;

    default:
        return false;
    }

}
