#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"
#include "../client/ClientNPCType.h"
#include "../client/ui/List.h"
#include "../client/ui/Label.h"

TEST("Sizeof empty list")
    // When a new List element is created
    List l(Rect(0, 0, 100, 100));

    // Then its size is 0
    return l.size() == 0;
TEND

TEST("Size of nonempty list")
    // Given an empty List element
    List l(Rect(0, 0, 100, 100));

    // When an element is added
    l.addChild(new Label(Rect(), "asdf"));

    // Then its size is 1
    return l.size() == 1;
TEND

QUARANTINED_TEST("View merchant slots in window")
    // Given a logged-in client and an object with merchant slots
    TestServer s = TestServer::WithData("merchant");
    TestClient c = TestClient::WithData("merchant");
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

    // When the client opens the object's window
    cObj->onRightClick(c.client());
    WAIT_UNTIL (cObj->window() != nullptr);
    // Wait until the merchant interface is drawn
    typedef const Element *ep_t;
    const ep_t &e = cObj->merchantSlotElements()[0];
    WAIT_UNTIL (e != nullptr);
    WAIT_UNTIL (e->changed() == false);
    // Wait until merchant-slot details are received from server, and the element constructed
    WAIT_UNTIL (e->children().size() > 0);

    // Then the client successfully redraws without crashing
    c.waitForRedraw();
    return true;;
TEND

TEST("New client can build default constructions")
    // Given a buildable brick wall object type with no pre-requisites
    TestServer s = TestServer::WithData("brick_wall");

    // When a client logs in
    TestClient c = TestClient::WithData("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    // His construction window contains at least one item
    bool constructionWindowIsEmpty = c.uiBuildList().empty();
    return ! constructionWindowIsEmpty;
TEND

TEST("New client has target UI hidden")
    // When a client logs in
    TestServer s;
    TestClient c;
    WAIT_UNTIL (s.users().size() == 1);

    // His targeting UI is hidden
    return c.target().panel()->visible() == false;
TEND
