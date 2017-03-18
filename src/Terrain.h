#ifndef TERRAIN_H
#define TERRAIN_H

#include <string>

class Terrain{
    std::string _tag;

protected:
    Terrain(){}

public:
    static Terrain *empty();
    static Terrain *withTag(const std::string &tag);

    const std::string &tag() const { return _tag; }
};

#endif
