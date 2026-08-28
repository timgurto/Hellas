#include "TestClient.h"
#include "TestFixtures.h"
#include "testing.h"
#include "TestServer.h"

TEST_CASE_METHOD(ServerAndClientWithData, "NPCs untarget dead players",
                 "[ai][death]") {
  GIVEN("A fox targeting a user but out of aggro range") {
    useData(R"(
      <npcType id="fox" attack="1" />
    )");

    server->addNPC("fox", {100, 0});

    auto &fox = server->getFirstNPC();

    fox.makeAwareOf(*user);
    WAIT_UNTIL(fox.target() == user);

    WHEN("the user dies") {
      user->kill();

      THEN("the fox is not targeting anything") {
        WAIT_UNTIL(fox.target() == nullptr);
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Players know their respawn points",
                 "[death]") {
  THEN("he knows his respawn point") {
    WAIT_UNTIL(client.respawnPoint() == user->respawnPoint());
  }
}
