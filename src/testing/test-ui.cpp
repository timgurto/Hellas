#include "Test.h"
#include "TestClient.h"
#include "TestServer.h"
#include "../client/ClientNPCType.h"
#include "../client/ui/List.h"
#include "../client/ui/Label.h"

TEST("Size of empty list")
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
    WAIT_UNTIL (s.entities().size() == 1);
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

    // Then his targeting UI is hidden
    return c.target().panel()->visible() == false;
TEND

TEST("Chat messages are added to chat log")
    // Given a logged-in client
    TestServer s;
    TestClient c;
    WAIT_UNTIL (s.users().size() == 1);

    // When he sends a message
    c.sendMessage(CL_SAY, "asdf");

    // Then his chat log contains at least one message
    WAIT_UNTIL(c.chatLog()->size() > 0);
    return true;
TEND

TEST("Objects show up on the map when a client logs in")
    // Given a server with rock objects;
    TestServer s = TestServer::WithData("basic_rock");

    // And a rock near the user spawn point
    s.addObject("rock", Point(10, 15));

    // When a client logs in
    TestClient c = TestClient::WithData("basic_rock");

    // The rock shows up on his map (in addition to the user himself)
    WAIT_UNTIL (c.mapPins().size() == 2);
TEND

TEST("A player shows up on his own map")
    // Given a server and client, and a 101x101 map on which players spawn at the center;
    TestServer s = TestServer::WithData("big_map");
    TestClient c = TestClient::WithData("big_map");

    // When the client is loaded
    c.waitForMessage(SV_LOCATION);

    // Then the map has one pin;
    WAIT_UNTIL (c.mapPins().size() == 1);

    // And that pin has the player's color
    const ColorBlock *pin = dynamic_cast<const ColorBlock *>(*c.mapPins().begin());
    ENSURE(pin != nullptr);
    ENSURE(pin->color() == Color::COMBATANT_SELF);

    // And that pin is in the center of the map
    const px_t
        midMapX = toInt(c->mapImage().width() / 2.0),
        midMapY = toInt(c->mapImage().height() / 2.0);
    const Point mapMidpoint(c->mapImage().width() / 2, c->mapImage().height() / 2);
    WAIT_UNTIL(pin->rect() == Rect(midMapX, midMapY, 1, 1));
TEND

ONLY_TEST("Window start uninitialized")
    // Given a server and client
    TestServer s;
    TestClient c;

    // When the client logs in
    WAIT_UNTIL(s.users().size() == 1);

    // Then the crafting window is uninitialized
    return ! c.craftingWindow()->isInitialized();
TEND

ONLY_TEST("Windows are initialized when used")
    // Given a server and client
    TestServer s;
    TestClient c;
    WAIT_UNTIL(s.users().size() == 1);

    // When the client opens the crafting window
    c.craftingWindow()->show();

    // Then it is initializezd
    WAIT_UNTIL(c.craftingWindow()->isInitialized());
TEND
