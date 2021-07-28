#include "TestClient.h"
#include "TestServer.h"

extern Renderer renderer;

TEST_CASE("Objects show up on the map when a client logs in") {
  // Given a server and client with rock objects;
  TestServer s = TestServer::WithData("basic_rock");
  TestClient c = TestClient::WithData("basic_rock");

  // And a rock near the user spawn point
  s.addObject("rock", {10, 15});

  // When the client finds out his location;
  CHECK(c.waitForMessage(SV_USER_LOCATION));

  // And he opens his map window
  c.mapWindow()->show();

  // The rock shows up on his map (in addition to the user himself)
  WAIT_UNTIL(c.mapPins().size() == 2);
}

TEST_CASE("A player shows up on his own map", "[.flaky]") {
  // Given a server and client, and a 101x101 map on which players spawn at the
  // center;
  TestServer s = TestServer::WithData("big_map");
  TestClient c = TestClient::WithData("big_map");

  // When the client finds out his location;
  CHECK(c.waitForMessage(SV_USER_LOCATION));

  // And he opens his map window
  c.mapWindow()->show();

  // Then the map has one pin;
  WAIT_UNTIL(c.mapPins().size() == 1);

  // And that pin has the player's color
  const ColorBlock *pin =
      dynamic_cast<const ColorBlock *>(*c.mapPins().begin());
  CHECK(pin != nullptr);
  CHECK(pin->color() == Color::COMBATANT_SELF);

  // And that pin is 1x1 and in the center of the map;
  const auto &mapImage = Client::images.map;
  const px_t midMapX = toInt(mapImage.width() / 2.0),
             midMapY = toInt(mapImage.height() / 2.0);
  const MapPoint mapMidpoint(mapImage.width() / 2, mapImage.height() / 2);
  WAIT_UNTIL(pin->rect() == ScreenRect(midMapX, midMapY, 1, 1));

  // And the map has one pin outline;
  WAIT_UNTIL(c.mapPinOutlines().size() == 1);

  // And that outline has the outline color
  const ColorBlock *outline =
      dynamic_cast<const ColorBlock *>(*c.mapPinOutlines().begin());
  CHECK(outline != nullptr);
  CHECK(outline->color() == Color::UI_OUTLINE);

  // And that outline is 3x3 and in the center of the map;
  CHECK(outline->rect() == ScreenRect(midMapX - 1, midMapY - 1, 3, 3));

  // And pixels of the player's color and border color are in the correct places
  REPEAT_FOR_MS(100);
  px_t xInScreen = midMapX + toInt(c.mapWindow()->position().x) + 1,
       yInScreen = midMapY + toInt(c.mapWindow()->position().y) + 2 +
                   Window::HEADING_HEIGHT;
  CHECK(renderer.getPixel(xInScreen, yInScreen) == Color::COMBATANT_SELF);
  CHECK(renderer.getPixel(xInScreen - 1, yInScreen) == Color::UI_OUTLINE);
}

TEST_CASE("Other players show up on the map") {
  // Given a server and two clients
  TestServer s;
  TestClient c1, c2;

  // When both clients log in;
  s.waitForUsers(2);

  // And the first client opens his map
  c1.mapWindow()->show();

  // Then there are two pins visible
  WAIT_UNTIL(c1.mapPins().size() == 2);
}

TEST_CASE("When a player declares war, his map pin changes color",
          "[war][.flaky]") {
  // Given a server with two clients;
  TestServer s;
  auto c = TestClient{};
  auto c2 = TestClient::WithUsername("Duteros");

  // And the first has his map open;
  s.waitForUsers(2);
  c.mapWindow()->show();

  // And sees two map pins
  WAIT_UNTIL(c.mapPins().size() == 2);

  // When the first declares war on the second;
  c.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Duteros");

  // And the war is confirmed to him;
  WAIT_UNTIL(c.otherUsers().size() == 1);
  const auto &duteros = c.getFirstOtherUser();
  WAIT_UNTIL(c.isAtWarWith(duteros));

  // And the map refreshes
  REPEAT_FOR_MS(200);

  // Then his map has one blue pin and one red pin
  bool bluePinExists = false, redPinExists = false;
  for (const auto *elemPin : c.mapPins()) {
    const auto &pin = dynamic_cast<const ColorBlock &>(*elemPin);
    if (pin.color() == Color::COMBATANT_SELF)
      bluePinExists = true;
    else if (pin.color() == Color::COMBATANT_ENEMY)
      redPinExists = true;
  }
  CHECK(bluePinExists);
  CHECK(redPinExists);
}
