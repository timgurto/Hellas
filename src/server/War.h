#ifndef WAR_H
#define WAR_H

#include <map>
#include <string>

class Wars{
public:
    typedef std::string Belligerent;
    typedef std::pair<Belligerent, Belligerent> Belligerents;

    void declare(const Belligerent &a, const Belligerent &b);
    bool isAtWar(const Belligerent &a, const Belligerent &b) const;

private:
    std::multimap<Belligerent, Belligerent> container;

    static Belligerents sortAlphabetically(const Belligerent &a, const Belligerent &b);
};

#endif
