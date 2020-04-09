#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("User and NPC overlap allowed") {
  GIVEN("a colliding NPC, and a user above it") {
    auto data = R"(
      <npcType id="monster" >
        <collisionRect x="-10" y="0" w="20" h="1" />
      </npcType>
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.addNPC("monster", {10, 20});
    s.waitForUsers(1);

    WHEN("the user tries to move through it") {
      c.sendMessage(CL_LOCATION, makeArgs(10, 30));

      THEN("he gets past the NPC") {
        const auto &user = s.getFirstUser();
        WAIT_UNTIL(user.location().y > 20);
      }
    }
  }
}
