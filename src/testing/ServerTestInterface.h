// (C) 2016 Tim Gurto

#ifndef SERVER_TEST_INTERFACE_H
#define SERVER_TEST_INTERFACE_H

#include "../server/Server.h"

// A wrapper of the server, with full access, used for testing.
class ServerTestInterface{
    Server _server;

public:
    void run();
    void stop();
    
    std::set<const ObjectType *> &objectTypes() { return _server._objectTypes; }
    Server::objects_t &objects() { return _server._objects; }
    std::set<ServerItem> &items() { return _server._items; }
    std::set<User> &users() { return _server._users; }

    void setMap(const std::vector<std::vector<size_t> > &map);
    void addObject(const std::string &typeName, const Point &loc);

    Server *operator->(){ return &_server; }
    void loadData(const std::string path){ _server.loadData(path); }
};

#endif
