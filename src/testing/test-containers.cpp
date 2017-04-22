#include "Test.h"
#include "TestServer.h"
#include "TestClient.h"

TEST("Object container empty check")
    TestServer s; 
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
    TestServer s;
    s.loadData("testing/data/dismantle");
    s.run();

    TestClient c = TestClient::Data("dismantle");
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

    return c.waitForMessage(SV_ACTION_STARTED);
TEND

TEST("Place item in object");
    TestServer s;
    s.loadData("testing/data/dismantle");
    s.run();

    TestClient c = TestClient::Data("dismantle");
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
    if (!c.waitForMessage(SV_INVENTORY))
        return false;

    // Try to put item in object
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_SWAP_ITEMS, makeArgs(0, 0, serial, 0));

    // Should be the alert that the object's inventory has changed
    return c.getNextMessage() == SV_INVENTORY;
TEND