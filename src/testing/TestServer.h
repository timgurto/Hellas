#ifndef TEST_SERVER_H
#define TEST_SERVER_H

#include "../server/Server.h"

// A wrapper of the server, with full access, used for testing.
class TestServer{

public:
    TestServer();
    static TestServer KeepingOldData();
    static TestServer WithData(const std::string &dataPath);
    static TestServer WithDataAndKeepingOldData(const std::string &dataPath);
    ~TestServer();

    // Move constructor/assignment
    TestServer(TestServer &rhs);
    TestServer &operator=(TestServer &rhs);
    
    std::set<const ObjectType *> &objectTypes() { return _server->_objectTypes; }
    Entities &entities() { return _server->_entities; }
    Entity::byX_t &entitiesByX() { return _server->_entitiesByX; }
    std::set<ServerItem> &items() { return _server->_items; }
    std::set<User> &users() { return _server->_users; }
    std::vector<Spawner> &spawners() { return _server->_spawners; }
    Wars &wars() { return _server->_wars; }
    Cities &cities() { return _server->_cities; }
    ObjectsByOwner &objectsByOwner() { return _server->_objectsByOwner; }
    Kings &kings() { return _server->_kings; }

    User &getFirstUser();
    Object &getFirstObject();
    NPC &getFirstNPC();
    ServerItem &getFirstItem();

    void addObject(const std::string &typeName, const MapPoint &loc = MapPoint{},
                   const std::string &owner = "");
    void addNPC(const std::string &typeName, const MapPoint &loc = MapPoint{});
    void removeEntity(Entity &entity) { _server->removeEntity(entity); }
    void waitForUsers(size_t numUsers) const;

    Server *operator->(){ return _server; }
    void loadData(const std::string path){ _server->loadData(path); }
    void sendMessage(const Socket &socket, MessageCode code, const std::string &args){
        _server->sendMessage(socket, code, args);
    }
    void changePlayerSpawn(const MapPoint &location, double range = 0){
        User::newPlayerSpawn = location; User::spawnRadius = range; }

    void nop(){ _server->mapX(); }

    User &findUser(const std::string &username);

private:
    Server *_server;
    TestServer(const std::string &dataPath);

    void run();
    void stop();
};

#endif
