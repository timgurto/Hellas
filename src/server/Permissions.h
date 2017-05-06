#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <string>

#include "City.h"

class User;

// Manages an object's access permissions
class Permissions{
public:
    enum OwnerType{
        NONE,
        PLAYER,
        CITY
    };
    struct Owner{
        Owner();
        OwnerType type;
        std::string name;
    };
    
    void setPlayerOwner(const std::string &username);
    void setCityOwner(const City::Name &cityName);
    bool hasOwner() const { return ! _owner.name.empty(); }
    bool isOwnedByPlayer(const std::string &username) const;
    bool isOwnedByCity(const City::Name &cityName) const;
    const Owner &owner() const;
    bool doesUserHaveAccess(const std::string &username) const;

private:
    Owner _owner;
};

#endif
