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
