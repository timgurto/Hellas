#include <cassert>
#include <thread>

#include "TestServer.h"
#include "testing.h"
#include "../Args.h"

extern Args cmdLineArgs;

TestServer::TestServer(){
    _server = new Server;
    _server->loadData("testing/data/minimal");
    run();
}

TestServer::TestServer(const std::string &dataPath){
    _server = new Server;
    _server->loadData("testing/data/minimal");
    _server->loadData("testing/data/" + dataPath);
    run();
}

TestServer TestServer::KeepingOldData(){
    cmdLineArgs.remove("new");
    TestServer s;
    cmdLineArgs.add("new");
    return s;
}

TestServer TestServer::WithDataAndKeepingOldData(const std::string &dataPath){
    cmdLineArgs.remove("new");
    TestServer s(dataPath);
    cmdLineArgs.add("new");
    return s;
}

TestServer TestServer::WithData(const std::string &dataPath){
    return TestServer(dataPath);
}

TestServer::~TestServer(){
    if (_server == nullptr)
        return;
    stop();
    delete _server;
}

TestServer::TestServer(TestServer &rhs):
_server(rhs._server){
    rhs._server = nullptr;
}

TestServer &TestServer::operator=(TestServer &rhs){
    if (this == &rhs)
        return *this;
    delete _server;
    _server = rhs._server;
    rhs._server = nullptr;
    return *this;
}

void TestServer::run(){
    Server &server = *_server;
    std::thread([& server](){ server.run(); }).detach();
    WAIT_UNTIL (_server->_running);
}

void TestServer::stop(){
    _server->_loop = false;
    WAIT_UNTIL (!_server->_running);
}

void TestServer::addObject(const std::string &typeName, const Point &loc,
                           const std::string &owner){
    const ObjectType *const type = _server->findObjectTypeByName(typeName);
    assert(type != nullptr);
    _server->addObject(type, loc, owner);
}

void TestServer::addNPC(const std::string &typeName, const Point &loc){
    const ObjectType *const type = _server->findObjectTypeByName(typeName);
    assert (type->classTag() == 'n');
    const NPCType *const npcType = dynamic_cast<const NPCType *const>(type);
    _server->addNPC(npcType, loc);
}

User &TestServer::findUser(const std::string &username) {
    auto usersByName = _server->_usersByName;
    auto it = usersByName.find(username);
    assert (it != usersByName.end());
    User *user = const_cast<User *>(it->second);
    return *user;
}

User &TestServer::getFirstUser() {
    assert(! _server->_users.empty());
    return const_cast<User &>(* _server->_users.begin());
}

Object &TestServer::getFirstObject() {
    assert(! _server->_entities.empty());
    auto it =_server-> _entities.begin();
    Entity *ent = *it;
    return * dynamic_cast<Object *>(ent);
}

NPC &TestServer::getFirstNPC() {
    assert(! _server->_entities.empty());
    auto it =_server-> _entities.begin();
    Entity *ent = *it;
    return * dynamic_cast<NPC *>(ent);
}

ServerItem &TestServer::getFirstItem() {
    assert(! _server->_items.empty());
    return const_cast<ServerItem &>(* _server->_items.begin());
}
