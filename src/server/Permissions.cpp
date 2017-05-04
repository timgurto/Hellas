#include "Permissions.h"
#include "User.h"

void Permissions::setOwner(const std::string &ownerName) {
    _ownerName = ownerName;
}

const std::string &Permissions::owner() const {
    return _ownerName;
}

bool Permissions::doesUserHaveAccess(const std::string &username) const{
    if (_ownerName.empty())
        return true;
    return _ownerName == username;
}
