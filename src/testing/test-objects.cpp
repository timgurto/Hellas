#include <cassert>
#include <csignal>
#include <cstdlib>
#include <SDL.h>

#include "ClientTestInterface.h"
#include "ServerTestInterface.h"
#include "Test.h"
#include "../client/ClientNPC.h"
#include "../client/ClientNPCType.h"

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
    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/basic_rock");
    c.run();

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = const_cast<User &>(*s.users().begin());
    user.updateLocation(Point(10, 10));

    // Add a single rock
    s.addObject("rock", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    //Gather
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));
    WAIT_UNTIL (user.action() == User::Action::GATHER) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    //Make sure user has item
    const Item &item = *s.items().begin();
    if (user.inventory()[0].first != &item)
        return false;

    //Make sure object no longer exists
    else if (!s.objects().empty())
        return false;

    return true;
TEND

TEST("View merchant slots in window")
    ServerTestInterface s;
    s.loadData("testing/data/merchant");
    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/merchant");
    c.run();

    // Move user to middle
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
    if (cObj == nullptr)
        return false;

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
    return true;;
TEND

/*
One gather worth of 1 million units of iron
1000 gathers worth of single rocks
This is to test the new gather algorithm, which would favor rocks rather than iron.
*/
TEST("Gather chance is by gathers, not quantity")
    ServerTestInterface s;
    s.loadData("testing/data/rare_iron");
    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/rare_iron");
    c.run();

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = const_cast<User &>(*s.users().begin());
    user.updateLocation(Point(10, 10));

    // Add a single iron deposit
    s.addObject("ironDeposit", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    //Gather
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_GATHER, makeArgs(serial));
    WAIT_UNTIL (user.action() == User::Action::GATHER) ; // Wait for gathering to start
    WAIT_UNTIL (user.action() == User::Action::NO_ACTION) ; // Wait for gathering to finish

    //Make sure user has a rock, and not the iron
    const ServerItem &item = *s.items().find(ServerItem("rock"));
    return user.inventory()[0].first == &item;
TEND

TEST("Object container empty check")
    ObjectType type("box");
    type.containerSlots(5);
    Object obj(&type, Point());
    if (!obj.isContainerEmpty())
        return false;
    ServerItem item("rock");
    obj.container()[1] = std::make_pair(&item, 1);
    return obj.isContainerEmpty() == false;
TEND

TEST("Dismantle an object with an inventory")
    ServerTestInterface s;
    s.loadData("testing/data/dismantle");
    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/dismantle");
    c.run();

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = const_cast<User &>(*s.users().begin());
    user.updateLocation(Point(10, 10));

    // Add a single box
    s.addObject("box", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    // Deconstruct
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_DECONSTRUCT, makeArgs(serial));

    return c.getNextMessage() == SV_ACTION_STARTED;
TEND

TEST("Place item in object");
    ServerTestInterface s;
    s.loadData("testing/data/dismantle");
    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/dismantle");
    c.run();

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = const_cast<User &>(*s.users().begin());
    user.updateLocation(Point(10, 10));

    // Add a single box
    s.addObject("box", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    // Give user a box item
    user.giveItem(&*s.items().begin());
    size_t msg = c.getNextMessage();
    if (msg != SV_INVENTORY)
        return false;

    // Try to put item in object
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_SWAP_ITEMS, makeArgs(0, 0, serial, 0));

    // Should be the alert that the object's inventory has changed
    return c.getNextMessage() == SV_INVENTORY;
TEND

TEST("Constructible NPC")
    ClientTestInterface c;
    
    // Load an item that refers to an object type, then an NPC type to define it
    c.loadData("testing/data/construct_an_npc");

    const ClientObjectType &objType = **c.objectTypes().begin();
    if (objType.classTag() != 'n')
        return false;

    // Check its health (to distinguish it from a plain ClientObject)
    const ClientNPCType &npcType = dynamic_cast<const ClientNPCType &>(objType);
    return npcType.maxHealth() == 5;
TEND

TEST("Players can attack immediately")
    ServerTestInterface s;
    s.loadData("testing/data/ant");
    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/ant");
    c.run();

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = const_cast<User &>(*s.users().begin());
    user.updateLocation(Point(10, 10));

    // Add NPC
    s.addNPC("ant", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    // Start attacking
    const auto objects = c.objects();
    const ClientObject *objP = objects.begin()->second;
    const ClientNPC &ant = dynamic_cast<const ClientNPC &>(*objP);
    size_t serial = ant.serial();
    c.sendMessage(CL_TARGET, makeArgs(serial));
    
    // NPC should be damaged very quickly
    REPEAT_FOR_MS(200)
        if (ant.health() < ant.npcType()->maxHealth())
            return true;
    return false;
TEND

TEST("Thin objects block movement")
    ServerTestInterface s;
    s.loadData("testing/data/thin_wall");
    s.run();

    ClientTestInterface c;
    c.loadData("testing/data/thin_wall");
    c.run();

    // Move user to middle, below wall
    WAIT_UNTIL(s.users().size() == 1);
    User &user = const_cast<User &>(*s.users().begin());
    user.updateLocation(Point(10, 15));

    // Add wall
    s.addObject("wall", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    // Try to move user up, through the wall
    REPEAT_FOR_MS(500) {
        c.sendMessage(CL_LOCATION, makeArgs(10, 5));
        if (user.location().y < 5.5)
            return false;
    }

    return true;
TEND
