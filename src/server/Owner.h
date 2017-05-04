#ifndef OWNER_H
#define OWNER_H

class User;

// Describes the owner of an object
class Owner{
public:
    Owner(): _exists(false) {}

    operator bool() const { return _exists; }
    void setOwner(const User *owningUser) { _exists = true; }

private:
    bool _exists;
};

#endif
