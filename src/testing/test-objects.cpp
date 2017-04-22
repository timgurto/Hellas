#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"
#include "../client/ClientNPCType.h"

TEST("View merchant slots in window")
    TestServer s;
    s.loadData("testing/data/merchant");
    s.run();

    TestClient c = TestClient::Data("merchant");
    c.run();

    // Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    const User &user = *s.users().begin();
    const_cast<User &>(user).updateLocation(Point(10, 10));

    // Add a single vending machine
    s.addObject("vendingMachine", Point(10, 10));
    WAIT_UNTIL (s.objects().size() == 1);
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

TEST("Constructible NPC")
    // Load an item that refers to an object type, then an NPC type to define it
    TestClient c = TestClient::Data("construct_an_npc");

    const ClientObjectType &objType = **c.objectTypes().begin();
    if (objType.classTag() != 'n')
        return false;

    // Check its health (to distinguish it from a plain ClientObject)
    const ClientNPCType &npcType = dynamic_cast<const ClientNPCType &>(objType);
    return npcType.maxHealth() == 5;
TEND

TEST("Thin objects block movement")
    TestServer s;
    s.loadData("testing/data/thin_wall");
    s.run();

    TestClient c = TestClient::Data("thin_wall");
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
