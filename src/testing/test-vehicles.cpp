#include "../server/Vehicle.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Custom vehicle speeds", "[vehicles][stats]") {
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
  GIVEN("a user driving a 200px/s vehicle") {
    auto data = R"(
      <terrain index="G" id="grass" />
      <list id="default" default="1" >
          <allow id="grass" />
      </list>
      <newPlayerSpawn x="10" y="10" range="0" />
      <size x="40" y="2" />
      <row    y="0" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <row    y="1" terrain = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG" />
      <objectType id="racecar" isVehicle="1" vehicleSpeed="2.5" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    c.sendLocationUpdatesInstantly();

    s.waitForUsers(1);
    const auto &user = s.getFirstUser();
    const auto &racecar = s.addObject("racecar", {10, 15}, c->username());

    c.sendMessage(CL_MOUNT, makeArgs(racecar.serial()));
    WAIT_UNTIL(c->character().isDriving());

    WHEN("he moves in a straight line") {
      const auto startingLocation = user.location();
      c.simulateKeypress(SDL_SCANCODE_RIGHT);

      AND_WHEN("one second passes") {
        REPEAT_FOR_MS(1000);

        THEN("he has moved 200px") {
          const auto distanceTraveled =
              distance(user.location(), startingLocation);
          CHECK_ROUGHLY_EQUAL(distanceTraveled, 200, 0.1);
        }
      }
    }
  }

  GIVEN("a non-vehicle object with a vehicle speed specified") {
    auto data = R"(
      <objectType id="mistake" vehicleSpeed="0" />
    )";
    auto c = TestClient::WithDataString(data);

    THEN("it doesn't crash the client") {}
  }
}

TEST_CASE("If a vehicle dies stop driving it", "[vehicles]") {
  GIVEN("a player driving a vehicle") {
    auto data = R"(
    <objectType id="horse" isVehicle="1" />
  )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    auto &horse = s.addObject("horse");
    s.waitForUsers(1);
    c.sendMessage(CL_MOUNT, makeArgs(horse.serial()));
    const auto &user = s.getFirstUser();
    WAIT_UNTIL(user.isDriving());
    WAIT_UNTIL(c->character().isDriving());

    WHEN("the vehicle dies") {
      horse.kill();

      THEN("he is no longer driving") {
        CHECK(!user.isDriving());

        auto &vehicle = dynamic_cast<Vehicle &>(horse);
        CHECK(vehicle.driver().empty());

        AND_THEN("he knows it") { WAIT_UNTIL(!c->character().isDriving()); }
      }
    }
  }
}
