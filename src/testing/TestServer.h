#ifndef TEST_SERVER_H
#define TEST_SERVER_H

#include "../server/Server.h"

// A wrapper of the server, with full access, used for testing.
class TestServer {
  enum NotRunning { NOT_RUNNING };

 public:
  TestServer();
  static TestServer KeepingOldData();
  static TestServer WithData(const std::string &dataPath);
  static TestServer WithDataString(const std::string &data);
  static TestServer WithDataAndKeepingOldData(const std::string &dataPath);
  static TestServer WithDataStringAndKeepingOldData(const std::string &data);
  ~TestServer();

  // Move constructor/assignment
  TestServer(TestServer &rhs);
  TestServer &operator=(TestServer &rhs);

  void loadData(const std::string path);
  void loadDataFromString(const std::string data);

  std::set<const ObjectType *> &objectTypes() { return _server->_objectTypes; }
  Entities &entities() { return _server->_entities; }
  Entity::byX_t &entitiesByX() { return _server->_entitiesByX; }
  std::set<ServerItem> &items() { return _server->_items; }
  const std::set<ServerItem> &items() const { return _server->_items; }
  std::set<User> &users() { return _server->_users; }
  std::vector<Spawner> &spawners() { return _server->_spawners; }
  Wars &wars() { return _server->_wars; }
  Cities &cities() { return _server->_cities; }
  ObjectsByOwner &objectsByOwner() { return _server->_objectsByOwner; }
  Kings &kings() { return _server->_kings; }
  const std::map<char, Terrain *> &terrainTypes() const {
    return _server->_terrainTypes;
  }

  User &getFirstUser();
  Object &getFirstObject();
  NPC &getFirstNPC();
  NPCType &getFirstNPCType();
  ServerItem &getFirstItem();
  const Quest &getFirstQuest();
  const BuffType &getFirstBuff();

  User &findUser(const std::string &username);
  const Quest &findQuest(const Quest::ID &questID) const;
  const ServerItem &findItem(const std::string &id) const;

  void addObject(const std::string &typeName, const MapPoint &loc = MapPoint{},
                 const std::string &owner = "");
  void addNPC(const std::string &typeName, const MapPoint &loc = MapPoint{});
  void removeEntity(Entity &entity) { _server->removeEntity(entity); }
  void waitForUsers(size_t numUsers) const;

  Server *operator->() { return _server; }
  void sendMessage(const Socket &socket, MessageCode code,
                   const std::string &args) {
    _server->sendMessage(socket, code, args);
  }

  void nop() { _server->map(); }

 private:
  Server *_server;

  enum StringType { DATA_PATH, DATA_STRING };
  TestServer(const std::string &string, StringType type);
  TestServer(NotRunning);

  void run();
  void stop();
};

#endif
