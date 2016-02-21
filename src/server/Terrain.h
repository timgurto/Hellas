// (C) 2016 Tim Gurto

#ifndef TERRAIN_H
#define TERRAIN_H

class Terrain{
    bool _isTraversable;

public:
    Terrain(bool isTraversable = true);

    bool isTraversable() const { return _isTraversable; }
};

#endif
