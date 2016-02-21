// (C) 2016 Tim Gurto

#ifndef TERRAIN_H
#define TERRAIN_H

class Terrain{
    char _index;
    bool _isTraversable;

public:
    Terrain(char index, bool isTraversable = true);

    bool isTraversable() const { return _isTraversable; }

    bool operator<(const Terrain &rhs) const;
};

#endif
