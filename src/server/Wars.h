#ifndef WARS_H
#define WARS_H

#include <map>
#include <string>
#include <vector>

class Wars{
public:
    typedef std::string Belligerent;
    typedef std::pair<Belligerent, Belligerent> Belligerents;
    typedef std::multimap<Belligerent, Belligerent> container_t;

    void declare(const Belligerent &a, const Belligerent &b);
    bool isAtWar(const Belligerent &a, const Belligerent &b) const;
    std::pair<container_t::const_iterator, container_t::const_iterator> getAllWarsInvolving(
            const Belligerent &a) const;
    
    std::multimap<Belligerent, Belligerent>::const_iterator begin() const { return container.begin(); }
    std::multimap<Belligerent, Belligerent>::const_iterator end() const { return container.end(); }

private:
    container_t container;
};

#endif
