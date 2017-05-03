#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"
#include "../client/ClientNPCType.h"
#include "../client/ui/List.h"
#include "../client/ui/Label.h"

TEST("List size")
    List l(Rect(0, 0, 100, 100));
    if (l.size() != 0)
        return false;
    l.addChild(new Label(Rect(), "asdf"));
    return l.size() == 1;
TEND

QUARANTINED_TEST("View merchant slots in window")
    TestServer s = TestServer::Data("merchant");
    TestClient c = TestClient::Data("merchant");

    // Move user to middle
    WAIT_UNTIL (s.users().size() == 1);
    User &user = s.getFirstUser();
    user.updateLocation(Point(10, 10));

    // Add a single vending machine
    s.addObject("vendingMachine", Point(10, 10));
    WAIT_UNTIL (s.objects().size() == 1);
    WAIT_UNTIL (c.objects().size() == 1);

    auto objects = c.objects();
    auto it = objects.begin();
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

TEST("New client can build default constructions")
    TestServer s = TestServer::Data("brick_wall");
    TestClient c = TestClient::Data("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    bool constructionWindowIsEmpty = c.uiBuildList().empty();
    return ! constructionWindowIsEmpty;
TEND
