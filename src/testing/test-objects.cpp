// (C) 2016 Tim Gurto

#include <cassert>
#include <csignal>
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
    s.setMap();
    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/basic_rock");
    c.run();

    //Move user to object
    WAIT_UNTIL (s.users().size() == 1);
    const User &user = *s.users().begin();
    const_cast<User &>(user).updateLocation(Point(10, 10));

    // Add a single rock
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

TEST("View merchant slots in window")
    ServerTestInterface s;
    s.loadData("testing/data/merchant");
    s.setMap();
    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/merchant");
    c.run();

    // Move user to object
    WAIT_UNTIL (s.users().size() == 1);
    const User &user = *s.users().begin();
    const_cast<User &>(user).updateLocation(Point(10, 10));


    // Add a single vending machine
    s.addObject("vendingMachine", Point(10, 10));
    assert(s.objects().size() == 1);
    WAIT_UNTIL (c.objects().size() == 1);

    auto it = c.objects().begin();
    size_t serial = it->first;
    ClientObject *cObj = it->second;

    // Open merchant object's window
    cObj->onRightClick(c.client());
    WAIT_UNTIL (cObj->window() != nullptr);

    // Wait until the merchant interface is drawn
    typedef const Element *ep_t;
    const ep_t &e = cObj->merchantSlotElements()[0];
    WAIT_UNTIL (e != nullptr);
    WAIT_UNTIL (e->changed() == false);

    // Wait until merchant-slot details are received from server, and the element constructed
    WAIT_UNTIL (e->children().size() > 0);

    // Expected fail-case crash will happen on redraw.
    c.waitForRedraw();

    c.stop();
    s.stop();

    return true;;
TEND
