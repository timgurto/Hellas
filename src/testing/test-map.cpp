#include "TestClient.h"
#include "TestFixtures.h"
#include "testing.h"
#include "TestServer.h"

extern Renderer renderer;

TEST_CASE_METHOD(ServerAndClientWithDataFiles,
                 "Objects show up on the map when a client logs in") {
  // Given a server and client with rock objects;
  useData("basic_rock");

  // And a rock near the user spawn point
  server->addObject("rock", {10, 15});

  // When the client finds out his location;
  CHECK(client->waitForMessage(SV_USER_LOCATION));

  // And he opens his map window
  client->mapWindow()->show();

  // The rock shows up on his map (in addition to the user himself)
  WAIT_UNTIL(client->mapPins().size() == 2);
}

TEST_CASE_METHOD(ServerAndClientWithDataFiles,
                 "A player shows up on his own map", "[.flaky]") {
  useData("big_map");

  // When the client finds out his location;
  CHECK(client->waitForMessage(SV_USER_LOCATION));

  // And he opens his map window
  client->mapWindow()->show();

  // Then the map has one pin;
  WAIT_UNTIL(client->mapPins().size() == 1);

  // And that pin has the player's color
  const ColorBlock *pin =
      dynamic_cast<const ColorBlock *>(*client->mapPins().begin());
  CHECK(pin != nullptr);
  CHECK(pin->color() == Color::COMBATANT_SELF);

  // And that pin is 1x1 and in the center of the map;
  const auto &mapImage = Client::images.map;
  const px_t midMapX = toInt(mapImage.width() / 2.0),
             midMapY = toInt(mapImage.height() / 2.0);
  const MapPoint mapMidpoint(mapImage.width() / 2, mapImage.height() / 2);
  WAIT_UNTIL(pin->rect() == ScreenRect(midMapX, midMapY, 1, 1));

  // And the map has one pin outline;
  WAIT_UNTIL(client->mapPinOutlines().size() == 1);

  // And that outline has the outline color
  const ColorBlock *outline =
      dynamic_cast<const ColorBlock *>(*client->mapPinOutlines().begin());
  CHECK(outline != nullptr);
  CHECK(outline->color() == Color::UI_OUTLINE);

  // And that outline is 3x3 and in the center of the map;
  CHECK(outline->rect() == ScreenRect(midMapX - 1, midMapY - 1, 3, 3));

  // And pixels of the player's color and border color are in the correct places
  REPEAT_FOR_MS(100);
  px_t xInScreen = midMapX + toInt(client->mapWindow()->position().x) + 1,
       yInScreen = midMapY + toInt(client->mapWindow()->position().y) + 2 +
                   Window::HEADING_HEIGHT;
  CHECK(renderer.getPixel(xInScreen, yInScreen) == Color::COMBATANT_SELF);
  CHECK(renderer.getPixel(xInScreen - 1, yInScreen) == Color::UI_OUTLINE);
}

TEST_CASE_METHOD(TwoClients, "Other players show up on the map") {
  // And the first client opens his map
  cAlice.mapWindow()->show();

  // Then there are two pins visible
  WAIT_UNTIL(cAlice.mapPins().size() == 2);
}

TEST_CASE_METHOD(TwoClients,
                 "When a player declares war, his map pin changes color",
                 "[war][.flaky]") {
  // And the first has his map open;
  cAlice.mapWindow()->show();

  // And sees two map pins
  WAIT_UNTIL(cAlice.mapPins().size() == 2);

  // When the first declares war on the second;
  cAlice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, cBob.name());

  // And the war is confirmed to him;
  WAIT_UNTIL(cAlice.otherUsers().size() == 1);
  const auto &otherUser = cAlice.getFirstOtherUser();
  WAIT_UNTIL(cAlice.isAtWarWith(otherUser));

  // And the map refreshes
  REPEAT_FOR_MS(200);

  // Then his map has one blue pin and one red pin
  bool bluePinExists = false, redPinExists = false;
  for (const auto *elemPin : cAlice.mapPins()) {
    const auto &pin = dynamic_cast<const ColorBlock &>(*elemPin);
    if (pin.color() == Color::COMBATANT_SELF)
      bluePinExists = true;
    else if (pin.color() == Color::COMBATANT_ENEMY)
      redPinExists = true;
  }
  CHECK(bluePinExists);
  CHECK(redPinExists);
}
