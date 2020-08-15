#include "../server/Groups.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Shared XP") {
  GIVEN("Alice, Bob, Charlie, and a critter") {
    auto data = R"(
      <npcType id="critter" />
    )";
    auto s = TestServer::WithDataString(data);
    auto cAlice = TestClient::WithUsernameAndDataString("Alice", data);
    auto cBob = TestClient::WithUsernameAndDataString("Bob", data);
    auto cCharlie = TestClient::WithUsernameAndDataString("Charlie", data);
    s.waitForUsers(3);

    auto &uAlice = s.findUser("Alice");
    auto &uBob = s.findUser("Bob");
    auto &uCharlie = s.findUser("Charlie");

    auto &critter = s.addNPC("critter", {10, 10});

    AND_GIVEN("Alice and Bob are in a group") {
      s->groups->createGroup(uAlice);
      s->groups->addToGroup(uBob, uAlice);

      WHEN("Alice kills the critter") {
        uAlice.setTargetAndAttack(&critter);

        THEN("Bob gets XP") { WAIT_UNTIL(uBob.xp() > 0); }
      }

      AND_GIVEN("Charlie is also in the group") {
        s->groups->addToGroup(uCharlie, uAlice);

        WHEN("Alice kills the critter") {
          uAlice.setTargetAndAttack(&critter);

          THEN("Charlie gets XP") { WAIT_UNTIL(uCharlie.xp() > 0); }
        }
      }
    }

    AND_GIVEN("Bob and Charlie are in a group") {
      s->groups->createGroup(uBob);
      s->groups->addToGroup(uCharlie, uBob);

      WHEN("Alice kills the critter") {
        uAlice.setTargetAndAttack(&critter);

        THEN("Bob gets no XP") {
          REPEAT_FOR_MS(100);
          CHECK(uBob.xp() == 0);
        }
      }
    }
  }
}

// Shared XP
// Shared XP only if nearby
// Round-robin loot
// If loot is left, then anyone [in group] can pick it up
// /roll
// Show group members on map
// Leader can kick/invite
// Group chat
// City chat
// Invite if target is not in a group
// If in a group, only leader can invite
// Disappears when down to one member
