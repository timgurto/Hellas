#include "TerrainList.h"

std::map<std::string, TerrainList> TerrainList::_lists;

void TerrainList::allow(char terrain){
    _isWhitelist = true;
    _list.insert(terrain);
}

void TerrainList::forbid(char terrain){
    _isWhitelist = false;
    _list.insert(terrain);
}

bool TerrainList::allows(char terrain) const{
    if (_isWhitelist)
        return _list.find(terrain) != _list.end();
    else
        return _list.find(terrain) == _list.end();
}
