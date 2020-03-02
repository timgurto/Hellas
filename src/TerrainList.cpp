#include "TerrainList.h"

#include "XmlReader.h"

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
      return _dummy;
    }
    return _lists.begin()->second;  // Use first alphabetical.
  }
}

void TerrainList::loadFromXML(XmlReader &xr) {
  std::map<std::string, char>
      terrainCodes;  // For easier lookup when compiling lists below.
  for (auto elem : xr.getChildren("terrain")) {
    char index;
    std::string id;
    if (!xr.findAttr(elem, "index", index)) continue;
    if (!xr.findAttr(elem, "id", id)) continue;
    terrainCodes[id] = index;
  }

  for (auto elem : xr.getChildren("list")) {
    std::string id;
    if (!xr.findAttr(elem, "id", id)) continue;
    TerrainList tl;

    if (!xr.findAttr(elem, "description", tl._description)) continue;

    for (auto terrain : xr.getChildren("allow", elem)) {
      std::string s;
      if (xr.findAttr(terrain, "id", s) &&
          terrainCodes.find(s) != terrainCodes.end())
        tl.allow(terrainCodes[s]);
    }

    for (auto terrain : xr.getChildren("forbid", elem)) {
      std::string s;
      if (xr.findAttr(terrain, "id", s) &&
          terrainCodes.find(s) != terrainCodes.end())
        tl.forbid(terrainCodes[s]);
    }

    addList(id, tl);

    size_t default = 0;
    if (xr.findAttr(elem, "default", default) && default == 1) setDefault(id);
  }
}
