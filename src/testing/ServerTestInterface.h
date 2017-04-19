#ifndef SERVER_TEST_INTERFACE_H
#define SERVER_TEST_INTERFACE_H

#include "../server/Server.h"

// A wrapper of the server, with full access, used for testing.
class ServerTestInterface{
    Server _server;

public:
    void run();
    void stop();

    ServerTestInterface();
    ~ServerTestInterface(){ stop(); }

    static ServerTestInterface *KeepOldData();
    
    std::set<const ObjectType *> &objectTypes() { return _server._objectTypes; }
    Server::objects_t &objects() { return _server._objects; }
    std::set<ServerItem> &items() { return _server._items; }
    std::set<User> &users() { return _server._users; }
    std::map<size_t, Spawner> &spawners() { return _server._spawners; }
    Wars &wars() { return _server._wars; }

    void addObject(const std::string &typeName, const Point &loc);
    void addNPC(const std::string &typeName, const Point &loc);

    Server *operator->(){ return &_server; }
    void loadData(const std::string path){ _server.loadData(path); }
    void sendMessage(const Socket &socket, MessageCode code, const std::string &args){
        _server.sendMessage(socket, code, args);
    }
};

#endif
