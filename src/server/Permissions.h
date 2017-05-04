#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <string>

class User;

// Manages an object's access permissions
class Permissions{
public:
    void setOwner(const User *owningUser);
    void setOwner(const std::string &ownerName);
    bool hasOwner() const { return ! _ownerName.empty(); }
    const std::string &owner() const;

private:
    std::string _ownerName;
};

#endif
