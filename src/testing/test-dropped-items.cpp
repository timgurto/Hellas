#include "../server/DroppedItem.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Dropping an item creates an object") {
  GIVEN("an item type") {
    auto data = R"(
      <item id="apple" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    AND_GIVEN("a user has an item") {
      auto &user = s.getFirstUser();
      user.giveItem(&s.getFirstItem());

      WHEN("he drops it") {
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("there's an entity") { WAIT_UNTIL(s.entities().size() == 1); }
      }
    }
  }
}

TEST_CASE("Name is correct on client") {
  GIVEN("Apple and Orange item types") {
    auto data = R"(
      <item id="apple" name="Apple" />
      <item id="orange" name="Orange" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    for (auto pair : c.items()) {
      auto id = pair.first;
      auto name = pair.second.name();

      AND_GIVEN("a user has a " + id) {
        user.giveItem(s->findItem(id));

        WHEN("he drops it") {
          c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));
          WAIT_UNTIL(c.entities().size() == 2);  // item + player

          THEN("the new entity is named \"" + name + "\"") {
            const auto &di = c.getFirstDroppedItem();
            CHECK(di.name() == name);
          }
        }
      }
    }
  }
}

TEST_CASE("Dropped items have correct serials in client") {
  GIVEN("an item type") {
    auto data = R"(
      <item id="apple" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    AND_GIVEN("a user has an item") {
      user.giveItem(&s.getFirstItem());

      WHEN("he drops it") {
        c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

        THEN("the client entity has the correct serial number") {
          WAIT_UNTIL(s.entities().size() == 1);
          const auto &serverEntity = s.getFirstDroppedItem();

          WAIT_UNTIL(c.entities().size() == 2);  // item + player
          const auto &clientEntity = c.getFirstDroppedItem();

          INFO("Client serial = "s + toString(clientEntity.serial()));
          INFO("Server serial = "s + toString(serverEntity.serial()));
          CHECK(clientEntity.serial() == serverEntity.serial());
        }
      }
    }
  }
}

TEST_CASE("Dropped items land near the dropping player") {
  GIVEN("an item type") {
    auto data = R"(
      <item id="apple" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    AND_GIVEN("a user is at a random location") {
      user.teleportTo(s->map().randomPoint());

      AND_GIVEN("he has an item") {
        user.giveItem(&s.getFirstItem());

        WHEN("he drops it") {
          c.sendMessage(CL_DROP, makeArgs(Serial::Inventory(), 0));

          THEN("it is within action range of him") {
            WAIT_UNTIL(s.entities().size() == 1);
            const auto &di = s.getFirstDroppedItem();

            auto distanceFromDropper =
                distance(di.collisionRect(), user.collisionRect());
            CHECK(distanceFromDropper < Server::ACTION_DISTANCE);

            AND_THEN("it is at the same location on the client") {
              WAIT_UNTIL(c.entities().size() == 2);  // item + player
              const auto &clientEntity = c.getFirstDroppedItem();

              CHECK(clientEntity.location() == di.location());
            }
          }
        }
      }
    }
  }
}
