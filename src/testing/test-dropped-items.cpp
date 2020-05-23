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
