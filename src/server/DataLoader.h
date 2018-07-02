#pragma once

#include <set>
#include <string>

class Server;
class XmlReader;

class DataLoader {
 public:
  using Directory = std::string;
  using XML = std::string;
  static DataLoader FromPath(Server &client, const Directory &path = "Data");
  static DataLoader FromString(Server &client, const XML &data);

  void load(bool keepOldData = false);

  void loadTerrainLists(XmlReader &reader);
  void loadObjectTypes(XmlReader &reader);
  void loadQuests(XmlReader &reader);
  void loadNPCTypes(XmlReader &reader);
  void loadItems(XmlReader &reader);
  void loadRecipes(XmlReader &reader);
  void loadSpells(XmlReader &reader);
  void loadBuffs(XmlReader &reader);
  void loadClasses(XmlReader &reader);
  void loadSpawners(XmlReader &reader);
  void loadMap(XmlReader &reader);

 private:
  DataLoader(Server &server);

  Server &_server;

  // Only one of these will be nonempty.
  Directory _path;
  XML _data;

  using LoadFunction = void (DataLoader::*)(XmlReader &);
  void loadFromAllFiles(LoadFunction load);

  using FilesList = std::set<std::string>;
  FilesList findDataFiles() const;
  FilesList _files;
};
