// (C) 2016 Tim Gurto

#include <cstdlib>
#include <SDL.h>

#include "ClientTestInterface.h"
#include "ServerTestInterface.h"
#include "Test.h"

TEST("Start and stop server")
    ServerTestInterface server;
    server.run();
    server.stop();
    return true;
TEND

TEST("Server::loadData() replaces, not adds")
    ServerTestInterface s;
    s.loadData("testing/data/basic_rock");
    s.loadData("testing/data/basic_rock");
    return s.objectTypes().size() == 1;
TEND

TEST("Client::loadData() replaces, not adds")
    ClientTestInterface c;
    c.loadData("testing/data/basic_rock");
    c.loadData("testing/data/basic_rock");
    return c.objectTypes().size() == 1;
TEND

TEST("Load object type naming an invalid item")
    ServerTestInterface s;
    s.loadData("testing/data/fake_item");
    auto it = s.items().find(ServerItem("fakeStone"));
    return it == s.items().end();
TEND

TEST("Gather an item from an object")
    ServerTestInterface s;
    s.loadData("testing/data/basic_rock");

    // Replace map
    std::vector<std::vector<size_t> > map(1);
    map[0] = std::vector<size_t>(1, 0);
    s.setMap(map);

    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/basic_rock");
    c.run();

    //Move user to object
    WAIT_UNTIL (s.users().size() == 1);
    const User &user = *s.users().begin();
    const_cast<User &>(user).updateLocation(Point(10, 10));

    // Replace objects and NPCs with a single rock
    s.addObject("rock", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    //Gather
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));
    WAIT_UNTIL (user.action() == User::Action::GATHER) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    bool ret = true;

    //Make sure user has item
    const Item &item = *s.items().begin();
    if (user.inventory()[0].first != &item)
        ret = false;

    //Make sure object no longer exists
    else if (!s.objects().empty())
        ret = false;

    c.stop();
    s.stop();

    return ret;
TEND
