#include "TerrainList.h"
#include "Server.h"

std::map<std::string, TerrainList> TerrainList::_lists;
const TerrainList *TerrainList::_default = nullptr;
TerrainList TerrainList::_dummy{};

void TerrainList::allow(char terrain) {
  _isWhitelist = true;
  _list.insert(terrain);
}

void TerrainList::forbid(char terrain) {
  _isWhitelist = false;
  _list.insert(terrain);
}

bool TerrainList::allows(char terrain) const {
  if (_isWhitelist)
    return _list.find(terrain) != _list.end();
  else
    return _list.find(terrain) == _list.end();
}

void TerrainList::setDefault(const std::string &id) { _default = findList(id); }

const TerrainList *TerrainList::findList(const std::string &id) {
  auto it = _lists.find(id);
  if (it == _lists.end())
    return nullptr;
  else
    return &it->second;
}

const TerrainList &TerrainList::defaultList() {
  if (_default != nullptr)
    return *_default;
  else {
    if (_lists.empty()) {
      SERVER_ERROR("Can't get default terrain list because there are no lists");
      return _dummy;
    }
    return _lists.begin()->second;  // Use first alphabetical.
  }
}
