#include "Test.h"
#include "TestServer.h"
#include "TestClient.h"

TEST("Object container empty check")
    TestServer s; 
    ObjectType type("box");
    type.addContainer(ContainerType::WithSlots(5));
    Object obj(&type, Point());
    if (!obj.container().isEmpty())
        return false;
    ServerItem item("rock");
    obj.container().at(1) = std::make_pair(&item, 1);
    return obj.container().isEmpty() == false;
TEND

TEST("Dismantle an object with an inventory")
    // Given a running server;
    TestServer s = TestServer::WithData("dismantle");
    // And a user at (10, 10);
    TestClient c = TestClient::WithData("dismantle");
    WAIT_UNTIL (s.users().size() == 1);
    User &user = s.getFirstUser();
    user.updateLocation(Point(10, 10));
    // And a box at (10, 10) that is deconstructible and has an empty inventory
    s.addObject("box", Point(10, 10));
    WAIT_UNTIL (c.objects().size() == 1);

    // When the user tries to deconstruct the box
    size_t serial = c.objects().begin()->first;
    c.sendMessage(CL_DECONSTRUCT, makeArgs(serial));

    // The deconstruction action successfully begins
    return c.waitForMessage(SV_ACTION_STARTED);
TEND

QUARANTINED_TEST("Place item in object");
    TestServer s = TestServer::WithData("dismantle");
    TestClient c = TestClient::WithData("dismantle");

    //Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = s.getFirstUser();
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