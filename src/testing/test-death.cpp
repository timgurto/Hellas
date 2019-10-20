#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("NPCs untarget dead players") {
  GIVEN("A fox targeting a user but out of aggro range") {
    auto data = R"(
      <npcType id="fox" attack="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addNPC("fox", {100, 0});

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    auto &fox = s.getFirstNPC();

    fox.makeAwareOf(user);
    WAIT_UNTIL(fox.target() == &user);

    WHEN("the user dies") {
      user.kill();

      THEN("the fox is not targeting anything") {
        WAIT_UNTIL(fox.target() == nullptr);
      }
    }
  }
}
