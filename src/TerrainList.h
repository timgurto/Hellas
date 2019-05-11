#ifndef TERRAIN_LIST_H
#define TERRAIN_LIST_H

#include <map>
#include <set>
#include <string>

class XmlReader;

/*
Describes an allowed set of terrain types.  Each object type will specify one,
which dictates what map tiles are valid locations for obejcts of that type.
*/

class TerrainList {
  static std::map<std::string, TerrainList> _lists;
  static const TerrainList *_default;
  static TerrainList _dummy;

  std::set<char> _list;
  bool _isWhitelist;  // true if whitelist; false if blacklist.
  static std::map<std::string, char> terrainCodes;

 public:
  void allow(char terrain);
  void forbid(char terrain);
  bool allows(char terrain) const;

  static void addList(const std::string &id, TerrainList &list) {
    _lists[id] = list;
  }
  static void clearLists() { _lists.clear(); }
  static void setDefault(const std::string &id);
  static const TerrainList *findList(const std::string &id);
  static const TerrainList &defaultList();

  static void loadFromXML(XmlReader &xr);
};

#endif
