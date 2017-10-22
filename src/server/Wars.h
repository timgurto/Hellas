#ifndef WARS_H
#define WARS_H

#include <map>
#include <string>
#include <vector>


struct Belligerent {
    enum Type {
        CITY,
        PLAYER
    };

    Type type;
    std::string name;

    Belligerent(const std::string &name = "", Type type = PLAYER) : type(type), name(name) {}
    Belligerent(const char *name, Type type = PLAYER) : type(type), name(name) {}

    bool operator==(const Belligerent &rhs) const;
    bool operator!=(const Belligerent &rhs) const;
    bool operator<(const Belligerent &rhs) const;

    void alertToWarWith(const Belligerent rhs) const;
};

class War {
    War(const Belligerent &b1, const Belligerent &b2);
    Belligerent b1, b2; // b1 < b2

    friend class Wars;

public:
    bool operator<(const War &rhs) const;
};

class Wars{
public:
    typedef std::set<War> container_t;

    void declare(const Belligerent &a, const Belligerent &b);
    bool isAtWar(Belligerent a, Belligerent b) const;
    void sendWarsToUser(const User &user, const Server &server) const;
    
    void writeToXMLFile(const std::string &filename) const;
    void readFromXMLFile(const std::string &filename);

private:
    container_t container;

    static void changePlayerBelligerentToHisCity(Belligerent &belligerent);
};

#endif
