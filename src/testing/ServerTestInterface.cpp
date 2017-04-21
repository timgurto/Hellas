#include <cassert>
#include <thread>

#include "ServerTestInterface.h"
#include "Test.h"
#include "../Args.h"

extern Args cmdLineArgs;

ServerTestInterface::ServerTestInterface(){
    _server = new Server;
    _server->loadData("testing/data/minimal");
    Server::newPlayerSpawnLocation = Point(10, 10);
    Server::newPlayerSpawnRange = 0;
}

ServerTestInterface ServerTestInterface::KeepOldData(){
    cmdLineArgs.remove("new");
    ServerTestInterface s;
    cmdLineArgs.add("new");
    return s;
}

ServerTestInterface::~ServerTestInterface(){
    if (_server == nullptr)
        return;
    stop();
    delete _server;
}

ServerTestInterface::ServerTestInterface(ServerTestInterface &rhs):
_server(rhs._server){
    rhs._server = nullptr;
}

ServerTestInterface &ServerTestInterface::operator=(ServerTestInterface &rhs){
    if (this == &rhs)
        return *this;
    delete _server;
    _server = rhs._server;
    rhs._server = nullptr;
    return *this;
}

void ServerTestInterface::run(){
    Server &server = *_server;
    std::thread([& server](){ server.run(); }).detach();
    WAIT_UNTIL (_server->_running);
}

void ServerTestInterface::stop(){
    _server->_loop = false;
    WAIT_UNTIL (!_server->_running);
}

void ServerTestInterface::addObject(const std::string &typeName, const Point &loc){
    const ObjectType *const type = _server->findObjectTypeByName(typeName);
    _server->addObject(type, loc);
}

void ServerTestInterface::addNPC(const std::string &typeName, const Point &loc){
    const ObjectType *const type = _server->findObjectTypeByName(typeName);
    assert(type->classTag() == 'n');
    const NPCType *const npcType = dynamic_cast<const NPCType *const>(type);
    _server->addNPC(npcType, loc);
}
