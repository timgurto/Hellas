#include <cassert>

#include "TerrainList.h"

std::map<std::string, TerrainList> TerrainList::_lists;
const TerrainList *TerrainList::_default = nullptr;

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

void TerrainList::setDefault(const std::string &id){
    auto it = _lists.find(id);
    if (it == _lists.end())
        _default = nullptr;
    else
        _default = &it->second;
}

const TerrainList &TerrainList::defaultList(){
    if (_default != nullptr)
        return *_default;
    else{
        assert(!_lists.empty());
        return _lists.begin()->second; // Use first alphabetical.
    }
}
