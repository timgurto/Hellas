// (C) 2016 Tim Gurto

#include <cstdlib>
#include <SDL.h>
#include <thread>

#include "Test.h"
#include "../client/Client.h"
#include "../server/Server.h"

TEST("Create server and client objects")
    Server s;
    Client c;
    return true;
TEND

TEST("Start and stop server")
    LogConsole log;
    Server server;
    Socket::debug = &log;
    std::thread([& server](){ server.run(); }).detach(); // Run server loop in new thread
    while (!server.running()) ; // Wait for server to begin running
    server.loop(false); // Stop server
    while (server.running()) ; // Wait for server to finish
    return true;
TEND

TEST("Server::loadData() replaces, not adds")
    Server s;
    s.loadData("testing/data/basic_rock");
    s.loadData("testing/data/basic_rock");
    return s._objectTypes.size() == 1;
TEND

TEST("Client::loadData() replaces, not adds")
    Client c;
    c.loadData("testing/data/basic_rock");
    c.loadData("testing/data/basic_rock");
    return c._objectTypes.size() == 1;
TEND

TEST("Load object type naming an invalid item")
    Server s;
    s.loadData("testing/data/fake_item");
    auto it = s._items.find(ServerItem("fakeStone"));
    return it == s._items.end();
TEND

TEST("Gather an item from an object")
    //Create server
    Server s;

    //Replace items, object types, objects with a single object yielding a single item
    s.loadData("testing/data/basic_rock");

    // Replace map
    s._mapX = 1;
    s._mapY = 1;
    s._map = std::vector<std::vector<size_t> >(1);
    s._map[0] = std::vector<size_t>(1, 0);

    std::thread([& s](){ s.run(); }).detach(); // Run server loop in new thread

    //Create client and replace data
    Client c;
    c.loadData("testing/data/basic_rock");

    // Wait for login
    std::thread([& c](){ c.run(); }).detach(); // Run client loop in new thread
    while (s._users.empty()) ;

    //Move user to object
    const User &user = *s._users.begin();
    const_cast<User &>(user).updateLocation(Point(10, 10));

    // Replace objects and NPCs with a single rock
    while (!s._objects.empty())
        s.removeObject(**s._objects.begin());
    while (! c._objects.empty()) ;
    const ObjectType *const rock = s.findObjectTypeByName("rock");
    s.addObject(rock, Point(10, 10));
    while (c._objects.size() != 1) ;

    //Gather
    Accessor<Client> client(c);
    size_t serial = client->_objects.begin()->first;
    client->sendMessage(CL_GATHER, makeArgs(serial));
    while (user.action() != User::Action::GATHER) ; // Wait for gathering to start
    while (user.action() != User::Action::NO_ACTION) ; // Wait for gathering to finish

    //Make sure user has item
    const Item &item = *s._items.begin();
    if (user.inventory()[0].first != &item)
        return false;

    //Make sure object no longer exists
    if (!s._objects.empty())
        return false;

    return true;
TEND
