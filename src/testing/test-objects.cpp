#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Damaged objects can't be deconstructed", "[damage-on-use]") {
  GIVEN("a 'brick' object with 1 out of 2 health") {
    auto data = R"(
      <item id="brick" durability="2" />
      <objectType id="brick" deconstructs="brick">
        <durability item="brick" quantity="1"/>
      </objectType>
    )";
    TestServer s = TestServer::WithDataString(data);
    TestClient c = TestClient::WithDataString(data);

    s.addObject("brick", {10, 15});
    Object &brick = s.getFirstObject();
    brick.reduceHealth(1);
    REQUIRE(brick.health() == 1);

    WHEN("the user tries to deconstruct the brick") {
      c.sendMessage(CL_PICK_UP_OBJECT_AS_ITEM, makeArgs(brick.serial()));

      THEN("the object still exists") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(s.entities().empty());
      }
    }
  }
}

TEST_CASE("Objects have 1 health by default", "[loading][stats]") {
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
