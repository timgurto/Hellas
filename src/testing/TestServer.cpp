#include "TestServer.h"

#include <cassert>
#include <thread>

#include "../Args.h"
#include "testing.h"

extern Args cmdLineArgs;

TestServer::TestServer() {
  _server = new Server;
  _server->_isTestServer = true;
  DataLoader::FromPath(*_server, "testing/data/minimal").load();
  run();
}

TestServer::TestServer(NotRunning) {
  _server = new Server;
  _server->_isTestServer = true;
  DataLoader::FromPath(*_server, "testing/data/minimal").load();
}

TestServer::TestServer(const std::string &string, StringType type) {
  _server = new Server;
  _server->_isTestServer = true;
  DataLoader::FromPath(*_server, "testing/data/minimal").load();
  if (type == DATA_PATH)
    DataLoader::FromPath(*_server, "testing/data/" + string).load(true);
  else if (type == DATA_STRING)
    DataLoader::FromString(*_server, string);
  run();
}

void TestServer::loadData(const std::string path) {
  DataLoader::FromPath(*_server, "testing/data/" + path).load(true);
}

void TestServer::loadDataFromString(const std::string data) {
  DataLoader::FromString(*_server, data).load(true);
}

TestServer TestServer::KeepingOldData() {
  cmdLineArgs.remove("new");
  TestServer s;
  cmdLineArgs.add("new");
  return s;
}

TestServer TestServer::WithDataAndKeepingOldData(const std::string &dataPath) {
  cmdLineArgs.remove("new");
  TestServer s(dataPath, DATA_PATH);
  cmdLineArgs.add("new");
  return s;
}

TestServer TestServer::WithDataStringAndKeepingOldData(
    const std::string &data) {
  cmdLineArgs.remove("new");
  auto s = TestServer{NOT_RUNNING};
  DataLoader::FromString(*s._server, data).load(true);
  s.run();
  cmdLineArgs.add("new");
  return s;
}

TestServer TestServer::WithData(const std::string &dataPath) {
  return TestServer(dataPath, DATA_PATH);
}

TestServer TestServer::WithDataString(const std::string &data) {
  auto s = TestServer{NOT_RUNNING};
  DataLoader::FromString(*s._server, data).load(true);
  s.run();
  return s;
}

TestServer::~TestServer() {
  if (_server == nullptr) return;
  stop();
  delete _server;
}

TestServer::TestServer(TestServer &rhs) : _server(rhs._server) {
  rhs._server = nullptr;
}

TestServer &TestServer::operator=(TestServer &rhs) {
  if (this == &rhs) return *this;
  delete _server;
  _server = rhs._server;
  rhs._server = nullptr;
  return *this;
}

void TestServer::run() {
  auto &server = *_server;
  std::thread([&server]() { server.run(); }).detach();
  WAIT_UNTIL(server._running);
}

void TestServer::stop() {
  _server->_loop = false;
  WAIT_UNTIL(!_server->_running);
}

void TestServer::addObject(const std::string &typeName, const MapPoint &loc,
                           const std::string &ownerName) {
  const ObjectType *const type = _server->findObjectTypeByName(typeName);
  REQUIRE(type != nullptr);
  auto owner = Permissions::Owner{};
  if (!ownerName.empty()) {
    owner.type = Permissions::Owner::PLAYER;
    owner.name = ownerName;
  }
  _server->addObject(type, loc, owner);
}

void TestServer::addNPC(const std::string &typeName, const MapPoint &loc) {
  const ObjectType *const type = _server->findObjectTypeByName(typeName);
  REQUIRE(type->classTag() == 'n');
  const NPCType *const npcType = dynamic_cast<const NPCType *const>(type);
  _server->addNPC(npcType, loc);
}

void TestServer::waitForUsers(size_t numUsers) const {
  // Timeout must be longer than Connection::TIME_BETWEEN_CONNECTION_ATTEMPTS
  WAIT_UNTIL_TIMEOUT(_server->_users.size() == numUsers, 4000);
  for (auto &user : _server->_users) WAIT_UNTIL(user.isInitialised());
}

User &TestServer::findUser(const std::string &username) {
  auto usersByName = _server->_usersByName;
  auto it = usersByName.find(username);
  REQUIRE(it != usersByName.end());
  User *user = const_cast<User *>(it->second);
  return *user;
}

const Quest &TestServer::findQuest(const Quest::ID &questID) const {
  return _server->_quests[questID];
}

const ServerItem &TestServer::findItem(const std::string &id) const {
  return *_server->findItem(id);
}

User &TestServer::getFirstUser() {
  REQUIRE(!_server->_users.empty());
  return const_cast<User &>(*_server->_users.begin());
}

Object &TestServer::getFirstObject() {
  REQUIRE(!_server->_entities.empty());
  auto it = _server->_entities.begin();
  Entity *ent = *it;
  auto pObj = dynamic_cast<Object *>(ent);
  REQUIRE(pObj);
  return *pObj;
}

NPC &TestServer::getFirstNPC() {
  REQUIRE(!_server->_entities.empty());
  auto it = _server->_entities.begin();
  auto pEntity = *it;
  auto pNpc = dynamic_cast<NPC *>(pEntity);
  REQUIRE(pNpc);
  return *pNpc;
}

NPCType &TestServer::getFirstNPCType() {
  REQUIRE(!_server->_objectTypes.empty());
  auto it = _server->_objectTypes.begin();
  auto *pObjType = const_cast<ObjectType *>(*it);
  auto pNpcType = dynamic_cast<NPCType *>(pObjType);
  REQUIRE(pNpcType);
  return *pNpcType;
}

ServerItem &TestServer::getFirstItem() {
  REQUIRE(!_server->_items.empty());
  return const_cast<ServerItem &>(*_server->_items.begin());
}

const Quest &TestServer::getFirstQuest() {
  REQUIRE(!_server->_quests.empty());
  return _server->_quests.begin()->second;
}

const BuffType &TestServer::getFirstBuff() {
  REQUIRE(!_server->_buffTypes.empty());
  return _server->_buffTypes.begin()->second;
}

const ClassType &TestServer::getFirstClass() {
  REQUIRE(!_server->_classes.empty());
  return _server->_classes.begin()->second;
}
