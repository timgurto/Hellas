#ifndef TERRAIN_LIST_H
#define TERRAIN_LIST_H

/*
Describes an allowed set of terrain types.  Each object type will specify one, which dictates what
map tiles are valid locations for obejcts of that type.
*/

class TerrainList{
public:
    bool allows(char terrain) const { return true; }
};

#endif
