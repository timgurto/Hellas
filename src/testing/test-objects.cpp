#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Damaged objects can't be deconstructed", "[damage-on-use]") {
  GIVEN("a 'brick' object with 1 out of 2 health") {
    useData(R"(
      <item id="brick" />
      <objectType id="brick" deconstructs="brick" maxHealth="2" />
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

TEST_CASE_METHOD(ServerAndClientWithData, "Objects are level 1 by default",
                 "[loading]") {
  GIVEN("an object") {
    useData(R"(
      <objectType id="A" />
    )");
    const auto &serverObject = server->addObject("A", {10, 15});

    THEN("it is level 1") { CHECK(serverObject.level() == 1); }
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

TEST_CASE_METHOD(ServerAndClientWithData, "Object-health categories",
                 "[stats]") {
  GIVEN("a house type whose health category specifies 100 health") {
    useData(R"(
      <objectHealthCategory id="building" maxHealth="100" />
      <objectType id="house" healthCategory="building" />
    )");

    WHEN("a house is spawned") {
      const auto &house = server->addObject("house", {10, 10});

      THEN("it has 100 health") {
        CHECK(house.health() == 100);

        AND_THEN("it has 100 health on the client") {
          CHECK(client->waitForFirstObject().health() == 100);
        }
      }
    }
  }

  GIVEN("a flower type whose health category specifies 2 health") {
    useData(R"(
      <objectHealthCategory id="plant" maxHealth="2" />
      <objectType id="flower" healthCategory="plant" />
    )");

    WHEN("a flower is spawned") {
      const auto &flower = server->addObject("flower", {10, 10});

      THEN("it has 2 health") {
        CHECK(flower.health() == 2);

        AND_THEN("it has 2 health on the client") {
          CHECK(client->waitForFirstObject().health() == 2);
        }
      }
    }
  }

  SECTION("Two categories are defined") {
    GIVEN("straw houses and brick houses have different health categories") {
      useData(R"(
        <objectHealthCategory id="straw" maxHealth="1" />
        <objectHealthCategory id="brick" maxHealth="10" />
        <objectType id="strawHouse" healthCategory="straw" />
        <objectType id="brickHouse" healthCategory="brick" />
      )");

      WHEN("both are spawned") {
        const auto &strawHouse = server->addObject("strawHouse", {10, 10});
        const auto &brickHouse = server->addObject("brickHouse", {10, 20});

        THEN("they have different health") {
          CHECK(strawHouse.health() != brickHouse.health());

          AND_THEN("they have different health on the client") {
            WAIT_UNTIL(client->objects().size() == 2);
            const auto &cStrawHouse = *client->objects()[strawHouse.serial()];
            const auto &cBrickHouse = *client->objects()[brickHouse.serial()];
            CHECK(cStrawHouse.health() != cBrickHouse.health());
          }
        }
      }
    }
  }

  SECTION("Default health for nonexistent categories") {
    GIVEN("a house type with a fake health category") {
      useData(R"(
        <objectType id="house" healthCategory="building" />
      )");

      WHEN("a house is spawned") {
        const auto &house = server->addObject("house", {10, 10});

        THEN("it has 1 health") {
          CHECK(house.health() == 1);

          AND_THEN("it has 1 health on the client") {
            CHECK(client->waitForFirstObject().health() == 1);
          }
        }
      }
    }
  }

  SECTION("Levels") {
    GIVEN("a house type with a category specifying level 2") {
      useData(R"(
        <objectHealthCategory id="building" maxHealth="100" />
        <objectType id="house" healthCategory="building" />
      )");

      WHEN("a house is spawned") {
        const auto &house = server->addObject("house", {10, 10});

        THEN("it is level 2") { CHECK(house.level() == 2); }
      }
    }
  }
}
