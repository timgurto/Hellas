// (C) 2016 Tim Gurto

#include <cassert>
#include <thread>

#include "ServerTestInterface.h"
#include "Test.h"

const std::vector<std::vector<char> > ServerTestInterface::TINY_MAP(1, std::vector<char>(1,0));

ServerTestInterface::ServerTestInterface(){
    loadMinimalData();
    setMap(); // A map is absolutely necessary, whether or not one is specified in map.xml
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

void ServerTestInterface::setMap(const std::vector<std::vector<char> > &map){
    assert(map.size() > 0);
    _server._mapX = map.size();

    assert(map[0].size() > 0);
    _server._mapY = map[0].size();
    for (auto col : map)
        assert(col.size() == _server._mapY);

    _server._map = map;
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
