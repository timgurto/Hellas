#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Only the tagging player gets kill XP", "[slow][remote]") {
  GIVEN("Alice and Bob are next to a seal") {
    auto data = R"(
      <newPlayerSpawn x="10" y="10" range="0" />
      <npcType id="seal" maxHealth="5" />
    )";
    auto s = TestServer::WithDataString(data);

    auto cAlice = TestClient::WithUsernameAndDataString("Alice", data);
    s.waitForUsers(1);
    auto &alice = *s->getUserByName("Alice");
    do {
      cAlice.sendMessage(CL_LOCATION, makeArgs(30, 30));
      REPEAT_FOR_MS(50);
    } while (alice.location() != MapPoint{30, 30});

    auto cBob = RemoteClient{"-username Bob"};
    s.waitForUsers(2);
    auto &bob = *s->getUserByName("Bob");

    s.addNPC("seal", {20, 20});
    const auto &seal = s.getFirstNPC();

    WHEN("Alice damages the seal") {
      cAlice.sendMessage(CL_TARGET_ENTITY, makeArgs(seal.serial()));
      WAIT_UNTIL(seal.health() < seal.stats().maxHealth);

      AND_WHEN("she stops attacking before it's dead") {
        cAlice.sendMessage(CL_TARGET_ENTITY, "0");
        REPEAT_FOR_MS(100);
        REQUIRE_FALSE(seal.isDead());

        AND_WHEN("Bob kills it") {
          s.sendMessage(bob.socket(), TST_SEND_THIS_BACK,
                        makeArgs(CL_TARGET_ENTITY, seal.serial()));
          WAIT_UNTIL_TIMEOUT(seal.isDead(), 10000);

          THEN("Bob doesn't get any XP") {
            REPEAT_FOR_MS(100);
            CHECK(bob.xp() == 0);

            AND_THEN("Alice does") { CHECK(alice.xp() > 0); }
          }
        }
      }
    }
  }
}
