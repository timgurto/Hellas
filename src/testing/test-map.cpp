#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"

TEST_CASE("Objects show up on the map when a client logs in", "[map]"){
    // Given a server and client with rock objects;
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");

    // And a rock near the user spawn point
    s.addObject("rock", Point(10, 15));
    
    // When the client finds out his location;
    CHECK(c.waitForMessage(SV_LOCATION));

    // And he opens his map window
    c.mapWindow()->show();

    // The rock shows up on his map (in addition to the user himself)
    WAIT_UNTIL (c.mapPins().size() == 2);
}

TEST_CASE("A player shows up on his own map", "[.flaky][map]"){
    // Given a server and client, and a 101x101 map on which players spawn at the center;
    TestServer s = TestServer::WithData("big_map");
    TestClient c = TestClient::WithData("big_map");

    // When the client finds out his location;
    CHECK(c.waitForMessage(SV_LOCATION));

    // And he opens his map window
    c.mapWindow()->show();


    // Then the map has one pin;
    WAIT_UNTIL(c.mapPins().size() == 1);

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

    // And that outline is 3x3 and in the center of the map;
    CHECK(outline->rect() == Rect(midMapX-1, midMapY-1, 3, 3));


    // And pixels of the player's color and border color are in the correct places
    REPEAT_FOR_MS(100);
    px_t
        xInScreen = midMapX + toInt(c.mapWindow()->position().x) + 1,
        yInScreen = midMapY + toInt(c.mapWindow()->position().y) + 2 + Window::HEADING_HEIGHT;
    CHECK(renderer.getPixel(xInScreen, yInScreen) == Color::COMBATANT_SELF);
    CHECK(renderer.getPixel(xInScreen-1, yInScreen) == Color::OUTLINE);
}

TEST_CASE("Other players show up on the map", "[map][remote]"){

    // Given a server and two clients
    TestServer s;
    TestClient c;
    RemoteClient rc;

    // When both clients log in;
    WAIT_UNTIL(s.users().size() == 2);

    // And the first client opens his map
    c.mapWindow()->show();

    // Then there are two pins visible
    WAIT_UNTIL(c.mapPins().size() == 2);
}
