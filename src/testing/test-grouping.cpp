#include "../client/UIGroup.h"
#include "../server/Groups.h"
#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Shared XP", "[grouping][leveling]") {
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
      s->groups->addToGroup("Bob", "Alice");

      WHEN("Alice kills the critter") {
        uAlice.setTargetAndAttack(&critter);

        THEN("Bob gets XP") { WAIT_UNTIL(uBob.xp() > 0); }
      }

      AND_GIVEN("Charlie is also in the group") {
        s->groups->addToGroup("Charlie", "Alice");

        WHEN("Alice kills the critter") {
          uAlice.setTargetAndAttack(&critter);

          THEN("Charlie gets XP") { WAIT_UNTIL(uCharlie.xp() > 0); }
        }
      }
    }

    AND_GIVEN("Bob and Charlie are in a group") {
      s->groups->addToGroup("Charlie", "Bob");

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
        s->groups->addToGroup("Bob", "Alice");

        THEN("Alice would receive half of her normal XP") {
          const int xpInGroupOf2 = uAlice.appropriateXPForKill(critter);
          CHECK(xpInGroupOf2 == normalXP / 2);
        }
      }

      WHEN("Alice is in a group of three") {
        s->groups->addToGroup("Bob", "Alice");
        s->groups->addToGroup("Charlie", "Alice");

        THEN("Alice would receive a third of her normal XP") {
          const int xpInGroupOf3 = uAlice.appropriateXPForKill(critter);
          CHECK_ROUGHLY_EQUAL(xpInGroupOf3, normalXP / 3, 0.1);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ThreeClients, "Inviting and accepting", "[grouping]") {
  THEN("Alice is not in a group") {
    CHECK_FALSE(server->groups->isUserInAGroup("Alice"));
  }

  WHEN("Bob sends an accept-invitation message") {
    cBob.sendMessage(CL_ACCEPT_GROUP_INVITATION);
    REPEAT_FOR_MS(100);

    THEN("Alice is not in a group") {
      CHECK_FALSE(server->groups->isUserInAGroup("Alice"));
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
        CHECK(server->groups->isUserInAGroup("Alice"));
        CHECK(server->groups->isUserInAGroup("Bob"));
        CHECK_FALSE(server->groups->isUserInAGroup("Charlie"));

        AND_THEN("Alice and Bob are in the same group") {
          auto group = server->groups->getUsersGroup("Alice");
          CHECK(group.count("Bob") == 1);
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
          CHECK_FALSE(server->groups->isUserInAGroup("Charlie"));
        }
      }
    }
  }
}

TEST_CASE_METHOD(TwoClients, "Invitations are case-insensitive", "[grouping]") {
  WHEN("Alice tries inviting \"bOb\" to a group") {
    cAlice.sendMessage(CL_INVITE_TO_GROUP, "bOb");
    THEN("Bob receives the invitation") {
      CHECK(cBob.waitForMessage(SV_INVITED_TO_GROUP));
    }
  }
}

TEST_CASE_METHOD(FourClients, "No duplicate groups", "[grouping]") {
  THEN("There are no groups") { CHECK(server->groups->numGroups() == 0); }

  GIVEN("Alice and Bob are in a group") {
    server->groups->addToGroup("Bob", "Alice");

    WHEN("Alice invites Charlie and he accepts") {
      cAlice.sendMessage(CL_INVITE_TO_GROUP, "Charlie");
      REPEAT_FOR_MS(100);
      cCharlie.sendMessage(CL_ACCEPT_GROUP_INVITATION);

      THEN("There is one group") {
        REPEAT_FOR_MS(100);
        CHECK(server->groups->numGroups() == 1);

        AND_THEN("Charlie is in a group") {
          CHECK(server->groups->isUserInAGroup("Charlie"));
        }
      }
    }

    AND_GIVEN("Charlie and Dan are in a group") {
      server->groups->addToGroup("Dan", "Charlie");

      THEN("There are two groups") { CHECK(server->groups->numGroups() == 2); }
    }
  }
}

TEST_CASE_METHOD(ThreeClients, "Grouped players can't accept invitations",
                 "[grouping]") {
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
            CHECK_FALSE(server->groups->isUserInAGroup("Charlie"));
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ThreeClients, "Grouped players can't be invited",
                 "[grouping]") {
  GIVEN("Alice and Bob are in a group") {
    server->groups->addToGroup("Bob", "Alice");

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

TEST_CASE_METHOD(ServerAndClient, "Group self-invites have no effect",
                 "[grouping]") {
  WHEN("the user invites himself to a group") {
    client.sendMessage(CL_INVITE_TO_GROUP, user->name());

    THEN("he receives no invitation") {
      CHECK_FALSE(client.waitForMessage(SV_INVITED_TO_GROUP));
    }
  }
}

TEST_CASE_METHOD(ThreeClients, "Clients know their teammates", "[grouping]") {
  THEN("Alice knows she has no other groupmates") {
    CHECK(cAlice->groupUI->otherMembers.empty());
  }

  WHEN("Alice and Bob are in a group") {
    server->groups->addToGroup("Bob", "Alice");

    THEN("Alice knows she has one groupmate, Bob") {
      WAIT_UNTIL(cAlice->groupUI->otherMembers.size() == 1);
      CHECK(cAlice->groupUI->otherMembers.count("Bob"s) == 1);

      AND_THEN("Bob knows that he has one groupmate, Alice") {
        WAIT_UNTIL(cBob->groupUI->otherMembers.size() == 1);
        CHECK(cBob->groupUI->otherMembers.count("Alice"s) == 1);

        AND_WHEN("Alice adds Charlie to the group") {
          server->groups->addToGroup("Charlie", "Alice");

          THEN("Bob knows that Charlie is in his group") {
            WAIT_UNTIL(cBob->groupUI->otherMembers.count("Charlie"s) == 1);
          }
        }
      }
    }
  }
}

TEST_CASE("A disconnected user remains in his group",
          "[grouping][connection]") {
  auto s = TestServer{};
  auto cBob = TestClient::WithUsername("Bob");

  // Given Alice is in a group
  {
    auto cAlice = TestClient::WithUsername("Alice");
    s.waitForUsers(2);

    s->groups->addToGroup("Alice", "Bob");

    // When Alice logs out and back in
  }
  s.waitForUsers(1);
  {
    auto cAlice = TestClient::WithUsername("Alice");
    s.waitForUsers(2);

    // Then Alice is still in a group
    CHECK(s->groups->isUserInAGroup("Alice"));

    // And she knows she has a groupmate
    WAIT_UNTIL(cAlice->groupUI->otherMembers.size() == 1);
  }
}

TEST_CASE_METHOD(TwoClients, "Group UI", "[grouping][ui]") {
  WHEN("Alice and Bob join a group") {
    server->groups->addToGroup("Alice", "Bob");

    THEN("In Alice's group UI, Bob's details are correct") {
      WAIT_UNTIL(cAlice->groupUI->otherMembers.size() == 1);
      const auto &alicesBobPanel = *cAlice->groupUI->otherMembers.begin();
      WAIT_UNTIL(alicesBobPanel.level == "1"s);
      WAIT_UNTIL(alicesBobPanel.health == bob->health());
      WAIT_UNTIL(alicesBobPanel.maxHealth == bob->stats().maxHealth);
      WAIT_UNTIL(alicesBobPanel.energy == bob->energy());
      WAIT_UNTIL(alicesBobPanel.maxEnergy == bob->stats().maxEnergy);

      AND_WHEN("Bob levels up") {
        bob->levelUp();

        THEN("his apparent level is accurate") {
          WAIT_UNTIL(alicesBobPanel.level == "2"s);
        }
      }

      AND_WHEN("Bob loses health and energy") {
        bob->reduceHealth(10);
        bob->reduceEnergy(10);

        THEN("his apparent health and energy are accurate") {
          WAIT_UNTIL(alicesBobPanel.health == bob->health());
          WAIT_UNTIL(alicesBobPanel.energy == bob->energy());
        }
      }

      AND_WHEN("Bob has 200 max health and energy") {
        alice->sendMessage({SV_MAX_HEALTH, makeArgs("Bob", 200)});
        alice->sendMessage({SV_MAX_ENERGY, makeArgs("Bob", 200)});

        THEN("his apparent max health and energy are 200") {
          WAIT_UNTIL(alicesBobPanel.maxHealth == 200);
          WAIT_UNTIL(alicesBobPanel.maxEnergy == 200);
        }
      }
    }
  }

  GIVEN("Bob is level 2") {
    bob->levelUp();

    WHEN("Alice and Bob join a group") {
      server->groups->addToGroup("Alice", "Bob");

      THEN("In Alice's group UI, Bob's level is 2") {
        WAIT_UNTIL(cAlice->groupUI->otherMembers.size() == 1);
        const auto &alicesBobPanel = *cAlice->groupUI->otherMembers.begin();
        WAIT_UNTIL(alicesBobPanel.level == "2"s);
      }
    }
  }
}

TEST_CASE_METHOD(ThreeClients, "Leaving a group", "[grouping]") {
  GIVEN("Alice, Bob and Charlie are in a group") {
    server->groups->addToGroup("Bob", "Alice");
    server->groups->addToGroup("Charlie", "Alice");
    WAIT_UNTIL(cAlice->groupUI->otherMembers.size() == 2);

    WHEN("Charlie sends a leave-group message") {
      cCharlie.sendMessage(CL_LEAVE_GROUP);

      THEN("he is not in a group") {
        WAIT_UNTIL(!server->groups->isUserInAGroup("Charlie"));

        AND_THEN("Alice still is") {
          CHECK(server->groups->isUserInAGroup("Alice"));

          AND_THEN("Alice's group has two members") {
            CHECK(server->groups->getGroupSize("Alice") == 2);

            AND_THEN("her UI shows one other member") {
              WAIT_UNTIL(cAlice->groupUI->otherMembers.size() == 1);
            }
          }
        }
      }
    }

    WHEN("Bob sends a leave-group message") {
      cBob.sendMessage(CL_LEAVE_GROUP);

      THEN("Bob is not in a group") {
        WAIT_UNTIL(!server->groups->isUserInAGroup("Bob"));

        AND_THEN("Bob's group UI is empty") {
          WAIT_UNTIL(cBob->groupUI->otherMembers.empty());
        }
      }
    }
  }

  SECTION("Existing group members in UI retian their stats") {
    // Given Alice, Bob and Dan are in a group
    auto dansHealth = Hitpoints{};
    {
      auto cDan = TestClient::WithUsername("Dan");
      server->groups->addToGroup("Bob", "Alice");
      server->groups->addToGroup("Dan", "Alice");

      // When Dan logs off
      server.waitForUsers(4);
      auto &dan = server.findUser("Dan");
      dansHealth = dan.health();
    }
    WAIT_UNTIL(cAlice.otherUsers().size() == 2);  // Bob, Charlie

    // And when Bob leaves the group
    cBob->sendMessage(CL_LEAVE_GROUP);

    // Then Alice's view of Dan still has its stats
    REPEAT_FOR_MS(100);
    const auto &alicesViewOfDan = *cAlice->groupUI->otherMembers.find({"Dan"});
    CHECK(alicesViewOfDan.health == dansHealth);
  }
}

TEST_CASE_METHOD(TwoClients, "Groups disappear when down to last member",
                 "[grouping]") {
  GIVEN("Alice and Bob are in a group") {
    server->groups->addToGroup("Bob", "Alice");

    WHEN("Alice leaves the group") {
      cAlice.sendMessage(CL_LEAVE_GROUP);

      THEN("Bob is not in a group") {
        WAIT_UNTIL(!server->groups->isUserInAGroup("Bob"));
      }
    }
  }
}

// Shared XP only if nearby
// Round-robin loot
// Master looter
// If loot is left, then anyone [in group] can pick it up
// Show group members on map
// Only the leader can kick/invite
// Group chat
// City chat
