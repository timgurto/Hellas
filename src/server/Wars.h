#ifndef WARS_H
#define WARS_H

#include <map>
#include <string>
#include <vector>

class Wars{
public:
    struct Belligerent{
        enum Type{
            CITY,
            PLAYER
        };

        Type type;
        std::string name;
        
        Belligerent(const std::string &name = "", Type type = PLAYER): type(type), name(name) {}
        Belligerent(const char *name, Type type = PLAYER): type(type), name(name) {}

        bool operator==(const Belligerent &rhs) const;
        bool operator!=(const Belligerent &rhs) const;
        bool operator<(const Belligerent &rhs) const;

        void alertToWarWith(const Belligerent rhs) const;
    };

    class War {
    public:
        War(const Belligerent &b1, const Belligerent &b2);
        const Belligerent &b1() const { return _b1; }
        const Belligerent &b2() const { return _b2; }
        bool operator<(const War &rhs) const;
    private:
        Belligerent _b1, _b2; // b1 < b2
    };

    //typedef std::pair<Belligerent, Belligerent> Belligerents;
    typedef std::set<War> container_t;

    void declare(const Belligerent &a, const Belligerent &b);
    bool isAtWar(Belligerent a, Belligerent b) const;
    void sendWarsToUser(const User &user, const Server &server) const;
    
    /*std::multimap<Belligerent, Belligerent>::const_iterator begin() const { return container.begin(); }
    std::multimap<Belligerent, Belligerent>::const_iterator end() const { return container.end(); }*/
    
    void writeToXMLFile(const std::string &filename) const;
    void readFromXMLFile(const std::string &filename);

private:
    container_t container;

    static void changePlayerBelligerentToHisCity(Belligerent &belligerent);
};

#endif
