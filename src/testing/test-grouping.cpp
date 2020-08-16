#include "../server/Groups.h"
#include "TestClient.h"
#include "TestFixtures.h"
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

    SECTION("XP is divided") {
      const int normalXP = uAlice.appropriateXPForKill(critter);

      WHEN("Alice is in a group of two") {
        s->groups->createGroup(uAlice);
        s->groups->addToGroup(uBob, uAlice);

        THEN("Alice would receive half of her normal XP") {
          const int xpInGroupOf2 = uAlice.appropriateXPForKill(critter);
          CHECK(xpInGroupOf2 == normalXP / 2);
        }
      }

      WHEN("Alice is in a group of three") {
        s->groups->createGroup(uAlice);
        s->groups->addToGroup(uBob, uAlice);
        s->groups->addToGroup(uCharlie, uAlice);

        THEN("Alice would receive a third of her normal XP") {
          const int xpInGroupOf3 = uAlice.appropriateXPForKill(critter);
          CHECK_ROUGHLY_EQUAL(xpInGroupOf3, normalXP / 3, 0.1);
        }
      }
    }
  }
}

TEST_CASE_METHOD(TwoUsersNamedAliceAndBob, "Inviting and accepting") {
  THEN("Alice is not in a group") {
    CHECK_FALSE(server->groups->isUserInAGroup(*alice));
  }

  WHEN("Bob sends an accept-invitation message") {
    cBob.sendMessage(CL_ACCEPT_GROUP_INVITATION);
    REPEAT_FOR_MS(100);

    THEN("Alice is not in a group") {
      CHECK_FALSE(server->groups->isUserInAGroup(*alice));
    }
  }

  AND_WHEN("Alice invites Bob to join a group") {
    cAlice.sendMessage(CL_INVITE_TO_GROUP, "Bob");

    AND_WHEN("Bob accepts") {
      REPEAT_FOR_MS(100);
      cBob.sendMessage(CL_ACCEPT_GROUP_INVITATION);

      THEN("Alice is in a group") {
        WAIT_UNTIL(server->groups->isUserInAGroup(*alice));
      }
    }
  }
}

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
