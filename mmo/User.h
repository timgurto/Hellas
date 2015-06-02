#ifndef USER_H
#define USER_H

#include <string>
#include <utility>
#include <windows.h>

// Stores information about a single user account for the server
class User{
public:
    User(const std::string &name, const std::pair<int, int> &location, const Socket &socket);

    const std::string &getName() const;
    const Socket &getSocket() const;

    std::string makeLocationCommand() const;

    std::pair<int, int> location;

private:
    std::string _name;
    Socket _socket;
};

#endif
