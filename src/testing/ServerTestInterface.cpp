// (C) 2016 Tim Gurto

#include <cassert>
#include <thread>

#include "ServerTestInterface.h"
#include "Test.h"

ServerTestInterface::ServerTestInterface(){
    loadMinimalData();
}

void ServerTestInterface::run(){
    Server &server = _server;
    std::thread([& server](){ server.run(); }).detach();
    WAIT_UNTIL (_server._running);
}

void ServerTestInterface::stop(){
    _server._loop = false;
    WAIT_UNTIL (!_server._running);
}

void ServerTestInterface::loadMinimalData(){
    _server.loadData("testing/data/minimal");
}

void ServerTestInterface::addObject(const std::string &typeName, const Point &loc){
    const ObjectType *const type = _server.findObjectTypeByName(typeName);
    _server.addObject(type, loc);
}

void ServerTestInterface::addNPC(const std::string &typeName, const Point &loc){
    const ObjectType *const type = _server.findObjectTypeByName(typeName);
    assert(type->classTag() == 'n');
    const NPCType *const npcType = dynamic_cast<const NPCType *const>(type);
    _server.addNPC(npcType, loc);
}
