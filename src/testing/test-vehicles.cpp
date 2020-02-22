#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Custom vehicle speeds") {
  GIVEN("a user driving a simple vehicle") {
    auto data = R"(
      <objectType id="wheelbarrow" isVehicle="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    c.sendLocationUpdatesInstantly();

    s.waitForUsers(1);
    const auto &user = s.getFirstUser();
    const auto &wheelbarrow =
        s.addObject("wheelbarrow", {10, 15}, c->username());

    c.sendMessage(CL_MOUNT, makeArgs(wheelbarrow.serial()));
    WAIT_UNTIL(c->character().isDriving());

    WHEN("he moves in a straight line") {
      const auto startingLocation = user.location();
      c.simulateKeypress(SDL_SCANCODE_RIGHT);

      AND_WHEN("one second passes") {
        REPEAT_FOR_MS(1000);

        THEN("he has moved as much as a non-mounted user would have") {
          const auto distanceTraveled =
              distance(user.location(), startingLocation);
          CHECK_ROUGHLY_EQUAL(distanceTraveled, user.stats().speed, 0.1);
        }
      }
    }
  }
}
