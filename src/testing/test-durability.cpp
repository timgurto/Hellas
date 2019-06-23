#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Items in inventory have full health") {
  GIVEN("an item type") {
    auto data = R"(
      <item id="apple" />
    )";
    auto s = TestServer::WithDataString(data);
    const auto &apple = s.getFirstItem();

    WHEN("a user is given one") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();

      user.giveItem(&apple);

      THEN("it has full health") {
        CHECK(user.inventory(0).first.health == ServerItem::MAX_HEALTH);
      }
    }
  }
}
