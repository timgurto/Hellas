#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Damaged objects can't be deconstructed", "[damage-on-use]") {
  GIVEN("a 'brick' object with 1 out of 2 health") {
    useData(R"(
      <item id="brick" durability="2" />
      <objectType id="brick" deconstructs="brick">
        <durability item="brick" quantity="1"/>
      </objectType>
    )");

    auto &brick = server->addObject("brick", {10, 15});
    brick.reduceHealth(1);
    REQUIRE(brick.health() == 1);

    WHEN("the user tries to deconstruct the brick") {
      client->sendMessage(CL_PICK_UP_OBJECT_AS_ITEM, makeArgs(brick.serial()));

      THEN("the object still exists") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(server->entities().empty());
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Specifying object health",
                 "[loading][stats]") {
  GIVEN("saplings have 2 max health") {
    useData(R"(
      <objectType id="sapling" maxHealth="2" />
    )");

    WHEN("a sapling is created") {
      const auto &sapling = server->addObject("sapling", {10, 10});
      const auto &serverObject = server->addObject("sapling", {10, 10});

      THEN("it has 2 health") { CHECK(serverObject.health() == 2); }

      WHEN("the client becomes aware of it") {
        WAIT_UNTIL(client->objects().size() == 1);
        const auto &clientObject = client->getFirstObject();

        THEN("the client knows it has 2 health") {
          CHECK(clientObject.health() == 2);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Objects have 1 health by default",
                 "[loading][stats]") {
  GIVEN("an object with no explicit health") {
    useData(R"(
      <objectType id="A" />
    )");

    const auto &serverObject = server->addObject("A", {10, 15});

    THEN("it has 1 health") { CHECK(serverObject.health() == 1); }

    WHEN("the client becomes aware of it") {
      WAIT_UNTIL(client->objects().size() == 1);
      const auto &clientObject = client->getFirstObject();

      THEN("the client knows it has 1 health") {
        CHECK(clientObject.health() == 1);
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
  }
}
