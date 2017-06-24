#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"
#include "../client/ClientNPCType.h"
#include "../client/ui/List.h"
#include "../client/ui/Label.h"

TEST_CASE("Size of empty list"){
    // When a new List element is created
    TestClient c;
    List l(Rect(0, 0, 100, 100));

    // Then its size is 0
    CHECK(l.size() == 0);
}

TEST_CASE("Size of nonempty list"){
    // Given an empty List element
    TestClient c;
    List l(Rect(0, 0, 100, 100));

    // When an element is added
    l.addChild(new Label(Rect(), "asdf"));

    // Then its size is 1
    CHECK(l.size() == 1);
}

TEST_CASE("View merchant slots in window", "[.flaky]"){
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
    REQUIRE(cObj != nullptr);

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
}

TEST_CASE("New client can build default constructions"){
    // Given a buildable brick wall object type with no pre-requisites
    TestServer s = TestServer::WithData("brick_wall");

    // When a client logs in
    TestClient c = TestClient::WithData("brick_wall");
    WAIT_UNTIL (s.users().size() == 1);

    // His construction window contains at least one item
    CHECK_FALSE(c.uiBuildList().empty());
}

TEST_CASE("New client has target UI hidden"){
    // When a client logs in
    TestServer s;
    TestClient c;
    WAIT_UNTIL (s.users().size() == 1);

    // Then his targeting UI is hidden
    CHECK_FALSE(c.target().panel()->visible());
}

TEST_CASE("Chat messages are added to chat log"){
    // Given a logged-in client
    TestServer s;
    TestClient c;
    WAIT_UNTIL (s.users().size() == 1);

    // When he sends a message
    c.sendMessage(CL_SAY, "asdf");

    // Then his chat log contains at least one message
    WAIT_UNTIL(c.chatLog()->size() > 0);
}

TEST_CASE("Objects show up on the map when a client logs in", "[map]"){
    // Given a server with rock objects;
    TestServer s = TestServer::WithData("basic_rock");

    // And a rock near the user spawn point
    s.addObject("rock", Point(10, 15));

    // When a client logs in
    TestClient c = TestClient::WithData("basic_rock");

    // The rock shows up on his map (in addition to the user himself)
    WAIT_UNTIL (c.mapPins().size() == 2);
}

TEST_CASE("A player shows up on his own map", "[map]"){
    // Given a server and client, and a 101x101 map on which players spawn at the center;
    TestServer s = TestServer::WithData("big_map");
    TestClient c = TestClient::WithData("big_map");

    // When the client is loaded
    CHECK(c.waitForMessage(SV_LOCATION));


    // Then the map has one pin;
    WAIT_UNTIL (c.mapPins().size() == 1);

    // And that pin has the player's color
    const ColorBlock *pin = dynamic_cast<const ColorBlock *>(*c.mapPins().begin());
    CHECK(pin != nullptr);
    CHECK(pin->color() == Color::COMBATANT_SELF);

    // And that pin is 1x1 and in the center of the map;
    const px_t
        midMapX = toInt(c->mapImage().width() / 2.0),
        midMapY = toInt(c->mapImage().height() / 2.0);
    const Point mapMidpoint(c->mapImage().width() / 2, c->mapImage().height() / 2);
    WAIT_UNTIL(pin->rect() == Rect(midMapX, midMapY, 1, 1));


    // And the map has one pin outline;
    WAIT_UNTIL (c.mapPinOutlines().size() == 1);

    // And that pin has the outline color
    const ColorBlock *outline = dynamic_cast<const ColorBlock *>(*c.mapPinOutlines().begin());
    CHECK(outline != nullptr);
    CHECK(outline->color() == Color::OUTLINE);

    // And that pin is 3x3 and in the center of the map;
    CHECK(outline->rect() == Rect(midMapX-1, midMapY-1, 3, 3));


    // And pixels of the player's color and border color are in the correct places
    c.mapWindow()->show();
    REPEAT_FOR_MS(100);
    px_t
        xInScreen = midMapX + toInt(c.mapWindow()->position().x) + 1,
        yInScreen = midMapY + toInt(c.mapWindow()->position().y) + 2 + Window::HEADING_HEIGHT;

    CHECK(renderer.getPixel(xInScreen, yInScreen) == Color::COMBATANT_SELF);
    CHECK(renderer.getPixel(xInScreen-1, yInScreen) == Color::OUTLINE);;

    // And there's a dark pixel where the outline should be
}

TEST_CASE("Windows start uninitialized"){
    // Given a server and client
    TestServer s;
    TestClient c;

    // When the client logs in
    WAIT_UNTIL(s.users().size() == 1);

    // Then the crafting window is uninitialized
    CHECK_FALSE(c.craftingWindow()->isInitialized());
}

TEST_CASE("Windows are initialized when used"){
    // Given a server and client
    TestServer s;
    TestClient c;
    WAIT_UNTIL(s.users().size() == 1);

    // When the client opens the crafting window
    c.craftingWindow()->show();

    // Then it is initializezd
    WAIT_UNTIL(c.craftingWindow()->isInitialized());
}

TEST_CASE("A visible window is fully-formed"){
    // Given a server and client;
    TestServer s;
    TestClient c;
    WAIT_UNTIL(s.users().size() == 1);

    // When the client opens the build window
    c.buildWindow()->show();

    // Then the build window has dimensions;
    WAIT_UNTIL(c.buildWindow()->Element::width() > 0);

    // And the heading has a texture
    WAIT_UNTIL(c.buildWindow()->getHeading()->texture());
}

TEST_CASE("Element gets initialized with Client", "[.flaky]"){
    CHECK_FALSE(Element::isInitialized()); // Depends on test order.
    Client c;
    WAIT_UNTIL(Element::isInitialized());
}

TEST_CASE("Gear window can be viewed"){
    TestServer s;
    TestClient c;
    c.gearWindow()->show();
    WAIT_UNTIL(c.gearWindow()->texture());
}

TEST_CASE("New clients survive recipe unlocks"){
    // Given a client and server
    TestServer s = TestServer::WithData("secret_bread");
    TestClient c = TestClient::WithData("secret_bread");
    WAIT_UNTIL(s.users().size() == 1);

    // When the server alerts the client to a recipe unlock
    s.sendMessage(s.getFirstUser().socket(), SV_NEW_RECIPES, makeArgs(1, "asdf"));

    // The client receives it.
    CHECK(c.waitForMessage(SV_NEW_RECIPES));
}
