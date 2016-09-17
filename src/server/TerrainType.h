// (C) 2016 Tim Gurto

#ifndef TERRAIN_TYPE_H
#define TERRAIN_TYPE_H

class TerrainType{
    bool _isTraversable;

public:
    TerrainType(bool isTraversable = true);

    bool isTraversable() const { return _isTraversable; }
};

#endif
