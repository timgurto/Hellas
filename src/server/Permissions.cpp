#include "Permissions.h"
#include "User.h"

void Permissions::setOwner(const User *owningUser) {
    _ownerName = owningUser->name();
}

void Permissions::setOwner(const std::string &ownerName) {
    _ownerName = ownerName;
}

const std::string &Permissions::owner() const {
    return _ownerName;
}
