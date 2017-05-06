#ifndef TEST_SERVER_H
#define TEST_SERVER_H

#include "../server/Server.h"

// A wrapper of the server, with full access, used for testing.
class TestServer{

public:
    TestServer();
    static TestServer KeepOldData();
    static TestServer Data(const std::string &dataPath);
    ~TestServer();

    // Move constructor/assignment
    TestServer(TestServer &rhs);
    TestServer &operator=(TestServer &rhs);
    
    std::set<const ObjectType *> &objectTypes() { return _server->_objectTypes; }
    Server::objects_t &objects() { return _server->_objects; }
    Object::byX_t &objectsByX() { return _server->_objectsByX; }
    std::set<ServerItem> &items() { return _server->_items; }
    std::set<User> &users() { return _server->_users; }
    std::map<size_t, Spawner> &spawners() { return _server->_spawners; }
    Wars &wars() { return _server->_wars; }
    Cities &cities() { return _server->_cities; }

    User &getFirstUser();
    Object &getFirstObject();
    ServerItem &getFirstItem();

    void addObject(const std::string &typeName, const Point &loc, const User *owner = nullptr);
    void addNPC(const std::string &typeName, const Point &loc);

    Server *operator->(){ return _server; }
    void loadData(const std::string path){ _server->loadData(path); }
    void sendMessage(const Socket &socket, MessageCode code, const std::string &args){
        _server->sendMessage(socket, code, args);
    }

    User &findUser(const std::string &username);

private:
    Server *_server;
    TestServer(const std::string &dataPath);

    void run();
    void stop();
};

#endif
