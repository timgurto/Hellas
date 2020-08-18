#include "../client/UIGroup.h"
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

TEST_CASE_METHOD(ThreeClients, "Inviting and accepting") {
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

  WHEN("Alice invites a nonexistent user to join a group") {
    cAlice.sendMessage(CL_INVITE_TO_GROUP, "Zeus");
    THEN("the server doesn't crash") {}
  }

  AND_WHEN("Alice invites Bob to join a group") {
    cAlice.sendMessage(CL_INVITE_TO_GROUP, "Bob");

    THEN("Bob receive an invitation") {
      CHECK(cBob.waitForMessage(SV_INVITED_TO_GROUP));
    }

    THEN("Alice does not receive an invitation") {
      CHECK_FALSE(cAlice.waitForMessage(SV_INVITED_TO_GROUP));
    }

    THEN("Alice does not receive a warning about Bob being in a group") {
      CHECK_FALSE(cAlice.waitForMessage(WARNING_USER_ALREADY_IN_A_GROUP));
    }

    AND_WHEN("Bob accepts") {
      REPEAT_FOR_MS(100);
      cBob.sendMessage(CL_ACCEPT_GROUP_INVITATION);

      THEN("Alice and Bob, but not Charlie, are in groups") {
        REPEAT_FOR_MS(100);
        CHECK(server->groups->isUserInAGroup(*alice));
        CHECK(server->groups->isUserInAGroup(*bob));
        CHECK_FALSE(server->groups->isUserInAGroup(*charlie));

        AND_THEN("Alice and Bob are in the same group") {
          auto group = server->groups->getUsersGroup(*alice);
          CHECK(group.find(bob) != group.end());
        }
      }
    }

    AND_WHEN("Charlie invites Alice to join a group") {
      cCharlie.sendMessage(CL_INVITE_TO_GROUP, "Alice");

      AND_WHEN("Bob accepts Alice's invitation") {
        REPEAT_FOR_MS(100);
        cBob.sendMessage(CL_ACCEPT_GROUP_INVITATION);

        THEN("Charlie is not in a group") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(server->groups->isUserInAGroup(*charlie));
        }
      }
    }
  }
}

TEST_CASE_METHOD(TwoClients, "Invitations are case-insensitive") {
  WHEN("Alice tries inviting \"bOb\" to a group") {
    cAlice.sendMessage(CL_INVITE_TO_GROUP, "bOb");
    THEN("Bob receives the invitation") {
      CHECK(cBob.waitForMessage(SV_INVITED_TO_GROUP));
    }
  }
}

TEST_CASE_METHOD(FourClients, "No duplicate groups") {
  THEN("There are no groups") { CHECK(server->groups->numGroups() == 0); }

  GIVEN("Alice and Bob are in a group") {
    server->groups->createGroup(*alice);
    server->groups->addToGroup(*bob, *alice);

    WHEN("Alice invites Charlie and he accepts") {
      cAlice.sendMessage(CL_INVITE_TO_GROUP, "Charlie");
      REPEAT_FOR_MS(100);
      cCharlie.sendMessage(CL_ACCEPT_GROUP_INVITATION);

      THEN("There is one group") {
        REPEAT_FOR_MS(100);
        CHECK(server->groups->numGroups() == 1);

        AND_THEN("Charlie is in a group") {
          CHECK(server->groups->isUserInAGroup(*charlie));
        }
      }
    }

    AND_GIVEN("Charlie and Dan are in a group") {
      server->groups->createGroup(*charlie);
      server->groups->addToGroup(*dan, *charlie);

      THEN("There are two groups") { CHECK(server->groups->numGroups() == 2); }
    }
  }
}

TEST_CASE_METHOD(ThreeClients, "Grouped players can't accept invitations") {
  WHEN("Alice invites Bob to a group") {
    cAlice.sendMessage(CL_INVITE_TO_GROUP, "Bob");

    AND_WHEN("Charlie invites Alice to a group") {
      cCharlie.sendMessage(CL_INVITE_TO_GROUP, "Alice");

      AND_WHEN("Bob accept's Alice's invitation") {
        REPEAT_FOR_MS(100);
        cBob.sendMessage(CL_ACCEPT_GROUP_INVITATION);

        AND_WHEN("Alice tries to accept Charlie's invitation") {
          REPEAT_FOR_MS(100);
          cAlice.sendMessage(CL_ACCEPT_GROUP_INVITATION);

          THEN("Charlie is not in a group") {
            REPEAT_FOR_MS(100);
            CHECK_FALSE(server->groups->isUserInAGroup(*charlie));
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ThreeClients, "Grouped players can't be invited") {
  GIVEN("Alice and Bob are in a group") {
    server->groups->createGroup(*alice);
    server->groups->addToGroup(*bob, *alice);

    WHEN("Charlie invites Alice to a group") {
      cCharlie.sendMessage(CL_INVITE_TO_GROUP, "Alice");

      THEN("Charlie receives a warning") {
        CHECK(cCharlie.waitForMessage(WARNING_USER_ALREADY_IN_A_GROUP));
      }

      THEN("Alice does not receive the invitation") {
        CHECK_FALSE(cAlice.waitForMessage(SV_INVITED_TO_GROUP));
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Group self-invites have no effect") {
  WHEN("the user invites himself to a group") {
    client.sendMessage(CL_INVITE_TO_GROUP, user->name());

    THEN("he receives no invitation") {
      CHECK_FALSE(client.waitForMessage(SV_INVITED_TO_GROUP));
    }
  }
}

TEST_CASE_METHOD(TwoClients, "Group UI") {
  THEN("Alice knows she has no other groupmates") {
    CHECK(cAlice->groupUI->otherMembers.empty());
  }

  WHEN("Alice and Bob are in a group") {
    server->groups->createGroup(*alice);
    server->groups->addToGroup(*bob, *alice);

    THEN("Alice knows she has one groupmate, Bob") {
      WAIT_UNTIL(cAlice->groupUI->otherMembers.size() == 1);
      CHECK(cAlice->groupUI->otherMembers.front() == "Bob");

      AND_THEN("Bob knows that he has one groupmate, Alice") {
        WAIT_UNTIL(cBob->groupUI->otherMembers.size() == 1);
        CHECK(cBob->groupUI->otherMembers.front() == "Alice");
      }
    }
  }
}

// All loot available to all group members
// /roll
// UI
// Ability to leave a group
// Disappears when down to one member

// Wait too long before accepting invitation
// Shared XP only if nearby
// Round-robin loot
// If loot is left, then anyone [in group] can pick it up
// Show group members on map
// Only the leader can kick/invite
// Group chat
// City chat
