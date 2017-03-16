#include <cassert>
#include <thread>

#include "ServerTestInterface.h"
#include "Test.h"

ServerTestInterface::ServerTestInterface(){
    _server.loadData("testing/data/minimal");
    Server::newPlayerSpawnLocation = Point(Server::TILE_W*5, Server::TILE_H*5);
    Server::newPlayerSpawnRange = 0;
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
