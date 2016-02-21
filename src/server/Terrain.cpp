// (C) 2016 Tim Gurto

#include "Terrain.h"

Terrain::Terrain(char index, bool isTraversable):
_index(index),
_isTraversable(isTraversable)
{}

bool Terrain::operator<(const Terrain &rhs) const{
    return _index < rhs._index;
}
