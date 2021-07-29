#include "TestServer.h"

#include <cassert>
#include <thread>

#include "../Args.h"
#include "../server/DroppedItem.h"
#include "../threadNaming.h"
#include "testing.h"

extern Args cmdLineArgs;

TestServer::TestServer() {
  _server = new Server;
  _server->_isTestServer = true;
  loadMinimalData();
  run();
}

TestServer::TestServer(NotRunning) {
  _server = new Server;
  _server->_isTestServer = true;
  loadMinimalData();
}

TestServer::TestServer(const std::string &string, StringType type) {
  _server = new Server;
  _server->_isTestServer = true;
  loadMinimalData();
  if (type == DATA_PATH)
    loadDataFromFiles(string);
  else if (type == DATA_STRING)
    loadDataFromString(string);

  run();
}

void TestServer::loadMinimalData() {
  auto dataLoader = DataLoader::FromPath(*_server, "testing/data/minimal");
  dataLoader.load();
  auto reader = XmlReader::FromFile("Data/core-spells.xml");
  dataLoader.loadSpells(reader);
}

void TestServer::loadDataFromFiles(const std::string path) {
  DataLoader::FromPath(*_server, "testing/data/" + path).load(true);
  _server->setDataSource(
      {Server::DataSource::FILES_PATH, "testing/data/" + path});
}

void TestServer::loadDataFromString(const std::string data) {
  DataLoader::FromString(*_server, data).load(true);
  _server->setDataSource({Server::DataSource::DATA_STRING, data});
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
  s.loadDataFromString(data);
  s.run();
  cmdLineArgs.add("new");
  return s;
}

TestServer TestServer::WithData(const std::string &dataPath) {
  return TestServer(dataPath, DATA_PATH);
}

TestServer TestServer::WithDataString(const std::string &data) {
  auto s = TestServer{NOT_RUNNING};
  s.loadDataFromString(data);
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
  std::thread([this]() {
    setThreadName("Server");
    _server->run();
  }).detach();
  WAIT_UNTIL(_server->_running);
}

void TestServer::stop() {
  _server->_loop = false;
  WAIT_UNTIL(!_server->_running);
}

Object &TestServer::addObject(const std::string &typeName, const MapPoint &loc,
                              const std::string &playerOwnerName) {
  auto owner = Permissions::Owner{};
  if (!playerOwnerName.empty()) {
    owner.type = Permissions::Owner::PLAYER;
    owner.name = playerOwnerName;
  }
  return addObject(typeName, loc, owner);
}

Object &TestServer::addObject(const std::string &typeName, const MapPoint &loc,
                              const Permissions::Owner &owner) {
  const ObjectType *const type = _server->findObjectTypeByID(typeName);
  REQUIRE(type != nullptr);
  return _server->addObject(type, loc, owner);
}

NPC &TestServer::addNPC(const std::string &typeName, const MapPoint &loc) {
  const ObjectType *const type = _server->findObjectTypeByID(typeName);
  REQUIRE(type->classTag() == 'n');
  const NPCType *const npcType = dynamic_cast<const NPCType *const>(type);
  return _server->addNPC(npcType, loc);
}

void TestServer::waitForUsers(size_t numUsers) const {
  // Timeout must be longer than Connection::TIME_BETWEEN_CONNECTION_ATTEMPTS
  WAIT_UNTIL_TIMEOUT(_server->_users.size() == numUsers, 4000);
  for (auto &user : _server->_users) WAIT_UNTIL(user.isInitialised());
}

void TestServer::saveData() {
  std::thread(Server::saveData, _server->_entities, _server->_wars,
              _server->_cities)
      .detach();
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

const BuffType &TestServer::findBuff(const std::string &id) const {
  const auto *buffType = _server->findBuff(id);
  REQUIRE(buffType);
  return *buffType;
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
  for (auto *pObj : _server->_entities) {
    auto pNpc = dynamic_cast<NPC *>(pObj);
    if (pNpc) return *pNpc;
  }
  FAIL("No NPCs on the server.");
  return NPC{nullptr, {}};
}

DroppedItem &TestServer::getFirstDroppedItem() {
  REQUIRE(!_server->_entities.empty());
  auto it = _server->_entities.begin();
  auto pEntity = *it;
  auto pDroppedItem = dynamic_cast<DroppedItem *>(pEntity);
  REQUIRE(pDroppedItem);
  return *pDroppedItem;
}

NPCType &TestServer::getFirstNPCType() {
  for (auto *pConstObjType : _server->_objectTypes) {
    auto *pObjType = const_cast<ObjectType *>(pConstObjType);
    auto *pNpcType = dynamic_cast<NPCType *>(pObjType);
    if (pNpcType) return *pNpcType;
  }
  FAIL("No NPC types on the server");
  return NPCType{{}};
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

Spawner &TestServer::getFirstSpawner() {
  REQUIRE(!_server->_spawners.empty());
  return _server->_spawners.front();
}

const Spell &TestServer::getFirstSpell() const {
  REQUIRE(!_server->_spells.empty());
  return *_server->_spells.begin()->second;
}
