#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Thin objects block movement") {
  // Given a server and client;
  auto s = TestServer::WithData("thin_wall");
  auto c = TestClient::WithData("thin_wall");

  // And a wall just above the user
  s.addObject("wall", {10, 5});

  // When the user tries to move up, through the wall
  s.waitForUsers(1);
  REPEAT_FOR_MS(500) {
    c.sendMessage(CL_LOCATION, makeArgs(10, 3));
    SDL_Delay(5);
  }

  // He fails
  auto &user = s.getFirstUser();
  CHECK(user.location().y > 4);
}

TEST_CASE("Dead objects don't block movement") {
  // Given a server and client;
  auto s = TestServer::WithData("thin_wall");
  auto c = TestClient::WithData("thin_wall");

  // And a wall just above the user;
  s.addObject("wall", {10, 5});

  // And that wall is dead
  s.getFirstObject().reduceHealth(1000000);

  // When the user tries to move up, through the wall
  s.waitForUsers(1);
  auto &user = s.getFirstUser();
  REPEAT_FOR_MS(3000) {
    c.sendMessage(CL_LOCATION, makeArgs(10, 3));

    if (user.location().y < 3.5) break;
  }
  // He succeeds
  CHECK(user.location().y < 3.5);
  ;
}

TEST_CASE("Damaged objects can't be deconstructed") {
  // Given a server and client;
  // And a 'brick' object type with 2 health;
  TestServer s = TestServer::WithData("pickup_bricks");
  TestClient c = TestClient::WithData("pickup_bricks");

  // And a brick object exists with only 1 health
  s.addObject("brick", {10, 15});
  Object &brick = s.getFirstObject();
  brick.reduceHealth(1);
  REQUIRE(brick.health() == 1);

  // When the user tries to deconstruct the brick
  c.sendMessage(CL_DECONSTRUCT, makeArgs(brick.serial()));

  // Then the object still exists
  REPEAT_FOR_MS(100);
  CHECK_FALSE(s.entities().empty());
}

TEST_CASE("Out-of-range objects are forgotten", "[.slow][culling][only]") {
  // Given a server and client with signpost objects;
  TestServer s = TestServer::WithData("signpost");
  TestClient c = TestClient::WithData("signpost");

  // And a signpost near the user spawn
  s.addObject("signpost", {10, 15});

  // And the client is aware of it
  s.waitForUsers(1);
  WAIT_UNTIL(c.objects().size() == 1);

  // When the client moves out of range of the signpost
  while (c->character().location().x < 1000) {
    c.sendMessage(CL_LOCATION, makeArgs(1010, 10));

    // Then he is no longer aware of it
    if (c.objects().size() == 0) break;
    SDL_Delay(5);
  }
  CHECK(c.objects().size() == 0);
}

TEST_CASE("Objects with no durability have 1 health in client") {
  // Given an object type A with no strength;
  auto data = R"(
      <objectType id="A" />
    )";
  TestServer s = TestServer::WithDataString(data);
  TestClient c = TestClient::WithDataString(data);

  // When an A object is added
  s.addObject("A", {10, 15});

  // And the client becomes aware of it
  s.waitForUsers(1);
  WAIT_UNTIL(c.objects().size() == 1);

  // Then the client knows it has 1 health
  auto &obj = c.getFirstObject();
  CHECK(obj.health() == 1);
}

TEST_CASE("Object durability") {
  GIVEN("n item of durability 3, and an object type of durability item*5") {
    auto data = R"(
      <item id="brick" durability="3" />
      <objectType id="wall">
        <durability item="brick" quantity="5"/>
      </objectType>
    )";
    TestServer s = TestServer::WithDataString(data);

    WHEN("one of the objects is added") {
      s.addObject("wall");

      THEN("it has 15 health") {
        const auto &wall = s.getFirstObject();
        CHECK(wall.health() == 15);
      }
    }
  }
}

TEST_CASE("Objects that disappear after a time") {
  GIVEN("an object that disappears after 1s") {
    auto data = R"(
      <objectType id="A" disappearAfter="1000" />
    )";
    TestServer s = TestServer::WithDataString(data);
    TestClient c = TestClient::WithDataString(data);

    s.addObject("A", {10, 15});

    WHEN("0.9s elapses") {
      REPEAT_FOR_MS(900);

      THEN("There is an object") { CHECK(s.entities().size() == 1); }

      AND_WHEN("Another 0.2s elapses") {
        REPEAT_FOR_MS(1100);

        THEN("There are no objects") { CHECK(s.entities().size() == 0); }
      }
    }
  }
}
