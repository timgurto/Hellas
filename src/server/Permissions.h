#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <string>

#include "City.h"

class Object;
class User;

// Manages an object's access permissions
class Permissions{
public:
    struct Owner{
        enum Type{
            NONE,
            PLAYER,
            CITY
        };
        Owner();
        Type type;
        std::string name;
        
        Owner(Type type, const std::string &name);
        const std::string &typeString() const;
        bool operator<(const Owner &rhs) const;
    };
    
    Permissions(const Object &parent): _parent(parent) {}

    void setPlayerOwner(const std::string &username);
    void setCityOwner(const City::Name &cityName);
    bool hasOwner() const;
    bool isOwnedByPlayer(const std::string &username) const;
    bool isOwnedByCity(const City::Name &cityName) const;
    const Owner &owner() const;
    bool doesUserHaveAccess(const std::string &username) const;
    void alertNearbyUsersToNewOwner() const;

private:
    Owner _owner;
    const Object &_parent;
};

#endif
