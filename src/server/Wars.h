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
        bool operator<(const Belligerent &rhs) const;

        void alertToWarWith(const Belligerent rhs) const;
    };

    typedef std::pair<Belligerent, Belligerent> Belligerents;
    typedef std::multimap<Belligerent, Belligerent> container_t;

    void declare(const Belligerent &a, const Belligerent &b);
    bool isAtWar(Belligerent a, Belligerent b) const;
    std::pair<container_t::const_iterator, container_t::const_iterator> getAllWarsInvolving(
            Belligerent a) const;
    
    std::multimap<Belligerent, Belligerent>::const_iterator begin() const { return container.begin(); }
    std::multimap<Belligerent, Belligerent>::const_iterator end() const { return container.end(); }

private:
    container_t container;

    static void changePlayerBelligerentToHisCity(Belligerent &belligerent);
};

#endif
