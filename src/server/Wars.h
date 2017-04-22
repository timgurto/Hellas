#ifndef WARS_H
#define WARS_H

#include <map>
#include <string>

class Wars{
public:
    typedef std::string Belligerent;
    typedef std::pair<Belligerent, Belligerent> Belligerents;

    void declare(const Belligerent &a, const Belligerent &b);
    bool isAtWar(const Belligerent &a, const Belligerent &b) const;
    
    std::multimap<Belligerent, Belligerent>::const_iterator begin() const { return container.begin(); }
    std::multimap<Belligerent, Belligerent>::const_iterator end() const { return container.end(); }

private:
    std::multimap<Belligerent, Belligerent> container;

    static Belligerents sortAlphabetically(const Belligerent &a, const Belligerent &b);
};

#endif
