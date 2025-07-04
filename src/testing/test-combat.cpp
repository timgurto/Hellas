#include "../client/ClientNPC.h"
#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Players can attack immediately",
                 "[combat]") {
  GIVEN("a targetable NPC") {
    useData(R"(
      <npcType id="ant" maxHealth="1" />
    )");
    auto &ant = server->addNPC("ant", {10, 10});

    WHEN("the player attacks it") {
      client->sendMessage(CL_TARGET_ENTITY, makeArgs(ant.serial()));

      THEN("the NPC should be damaged very quickly") {
        WAIT_UNTIL_TIMEOUT(ant.isMissingHealth(), 100);
      }
    }
  }
}

TEST_CASE_METHOD(TwoClients, "Only belligerents can target each other",
                 "[combat][war]") {
  WHEN("Alice tries to target Bob") {
    cAlice.sendMessage(CL_TARGET_PLAYER, "Bob");

    THEN("She is not targeting him") {
      REPEAT_FOR_MS(100);
      CHECK_FALSE(alice->target() == bob);
    }
  }

  GIVEN("Alice and Bob are at war") {
    cAlice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");

    AND_WHEN("Alice tries to target Bob") {
      cAlice.sendMessage(CL_TARGET_PLAYER, "Bob");

      THEN("Alice is targeting Bob") { WAIT_UNTIL(alice->target() == bob); }
    }
  }
}

TEST_CASE_METHOD(TwoClients, "Only belligerents can fight", "[combat][war]") {
  GIVEN("Alice and Bob are within melee range") {
    while (distance(*alice, *bob) > Server::ACTION_DISTANCE)
      alice->moveLegallyTowards(bob->location());

    WHEN("Alice tries to target Bob") {
      cAlice.sendMessage(CL_TARGET_PLAYER, "Bob");

      THEN("Bob doesn't lose health") {
        REPEAT_FOR_MS(500);
        CHECK(!bob->isMissingHealth());
      }
    }

    WHEN("Alice declares war on Bob") {
      cAlice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");

      AND_WHEN("Alice targets Bob") {
        cAlice.sendMessage(CL_TARGET_PLAYER, "Bob");

        THEN("Bob loses health") { WAIT_UNTIL(bob->isMissingHealth()); }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Attack rate is respected",
                 "[.flaky][combat]") {
  GIVEN("a wolf that hits for 1 every 100ms") {
    useData(R"(
      <npcType id="wolf" maxHealth="4000" attack="1" attackTime="100" />
    )");
    server->addNPC("wolf", {10, 10});

    AND_GIVEN("a user for whom every incoming attack is a hit") {
      auto oldStats = User::OBJECT_TYPE.baseStats();
      auto newStats = oldStats;
      newStats.critResist = 100;
      newStats.dodge = 0;
      newStats.hps = 0;
      User::OBJECT_TYPE.baseStats(newStats);
      user->updateStats();

      WHEN("1050ms elapse") {
        REPEAT_FOR_MS(1050);

        THEN("the user has taken exactly 10 damage") {
          CHECK(user->health() == user->stats().maxHealth - 10);
        }
      }

      User::OBJECT_TYPE.baseStats(oldStats);
    }
  }
}

TEST_CASE("Belligerents can attack each other's objects",
          "[combat][war][permissions]") {
  // Given a logged-in user;
  // And a vase object type with 1 health;
  TestServer s = TestServer::WithData("vase");
  TestClient c = TestClient::WithData("vase");

  // And a vase owned by Alice;
  s.addObject("vase", {10, 15}, "Alice");
  Object &vase = s.getFirstObject();
  REQUIRE(vase.health() == 1);

  // And that the user is at war with Alice
  const std::string &username = c.name();
  s.wars().declare(username, "Alice");

  // When he targets the vase
  s.waitForUsers(1);
  c.sendMessage(CL_TARGET_ENTITY, makeArgs(vase.serial()));

  // Then the vase has 0 health
  WAIT_UNTIL(vase.health() == 0);
}

TEST_CASE("Players can target distant entities") {
  // Given a server and client;
  TestServer s = TestServer::WithData("wolf");
  TestClient c = TestClient::WithData("wolf");

  // And a wolf NPC on the other side of the map
  s.addNPC("wolf", {200, 200});
  s.waitForUsers(1);
  const NPC &wolf = s.getFirstNPC();
  const User &user = s.getFirstUser();
  REQUIRE(distance(wolf, user) > Server::ACTION_DISTANCE);

  // When the client attempts to target the wolf
  WAIT_UNTIL(c.objects().size() == 1);
  ClientNPC &clientWolf = c.getFirstNPC();
  clientWolf.onRightClick();

  // Then his target is set to the wolf
  WAIT_UNTIL(user.target() == &wolf);
}

TEST_CASE_METHOD(TwoClients, "Clients receive nearby users' health values") {
  GIVEN("Alice and Bob are at war") {
    server.wars().declare("Alice", "Bob");

    AND_GIVEN("Alice is close to Bob") {
      while (distance(*alice, *bob) >= Server::ACTION_DISTANCE) {
        cAlice.sendMessage(CL_MOVE_TO,
                           makeArgs(bob->location().x, bob->location().y));
        SDL_Delay(5);
      }

      WHEN("Alice attacks Bob") {
        cAlice.sendMessage(CL_TARGET_PLAYER, "Bob");

        THEN("Alice sees that Bob is damaged") {
          WAIT_UNTIL(cAlice.otherUsers().size() == 1);
          const Avatar &localBob = cAlice.getFirstOtherUser();
          WAIT_UNTIL(localBob.health() < localBob.maxHealth());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "A player dying doesn't crash the server",
                 "[death]") {
  WHEN("a user dies") {
    user->reduceHealth(999999);

    THEN("the server doesn't crash") { server.nop(); }
  }
}

TEST_CASE("Civilian NPCs", "[combat]") {
  GIVEN("An NPC with \"isCivilian\" and 1 health") {
    auto s = TestServer::WithData("civilian");
    auto c = TestClient::WithData("civilian");

    s.addNPC("civilian", {10, 15});
    WAIT_UNTIL(c.objects().size() == 1);

    WHEN("a client attempts to attack it") {
      const auto &civilian = c.getFirstNPC();
      c.sendMessage(CL_TARGET_ENTITY, toString(civilian.serial()));

      THEN("it is still alive") {
        REPEAT_FOR_MS(500);
        CHECK(civilian.isAlive());
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "In-combat flag", "[combat]") {
  GIVEN("a user and a fox out of aggro range") {
    useData(R"(
      <npcType id="fox" attack="1" />
    )");

    auto &fox = server->addNPC("fox", {100, 0});

    THEN("The user is not in combat") { CHECK_FALSE(user->isInCombat()); }

    WHEN("the fox becomes aware of the user") {
      fox.makeAwareOf(*user);
      WAIT_UNTIL(fox.target() == user);
      THEN("the user is in combat") { WAIT_UNTIL(user->isInCombat()); }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Neutral NPCs", "[combat]") {
  GIVEN("a \"neutral\" NPC with attack, close to a user") {
    useData(R"(
      <npcType id="snake" attack="1" isNeutral="1" />
    )");

    server->addNPC("snake", {15, 10});

    THEN("the user doesn't get attacked") {
      REPEAT_FOR_MS(100);
      CHECK(!user->isMissingHealth());
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Targeting civilians after attacking",
                 "[combat]") {
  GIVEN("A user between an attackable sloth and a civilian maiden") {
    useData(R"(
      <newPlayerSpawn x="10" y="10" range="0" />
      <npcType id="sloth" maxHealth="1000" />
      <npcType id="maiden" maxHealth="1000" isCivilian="1" />
    )");

    const auto &sloth = server->addNPC("sloth", {10, 15});
    const auto &maiden = server->addNPC("maiden", {10, 5});

    WHEN("he attacks the sloth") {
      client->sendMessage(CL_TARGET_ENTITY, makeArgs(sloth.serial()));
      REPEAT_FOR_MS(100);

      AND_WHEN("he right-clicks the maiden") {
        WAIT_UNTIL(client->objects().size() == 2);
        auto &cMaiden = *client->objects()[maiden.serial()];
        cMaiden.onRightClick();

        THEN("the maiden doesn't take damage") {
          REPEAT_FOR_MS(3000);
          CHECK(!maiden.isMissingHealth());
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "XP from kills",
                 "[combat][leveling]") {
  GIVEN("normal and elite NPC types") {
    useData(R"(
      <npcType id="queenBee" maxHealth="1" boss="1" />
      <npcType id="soldierBee" maxHealth="1" elite="1" />
      <npcType id="workerBee" maxHealth="1" />
    )");

    AND_GIVEN("a normal NPC") {
      const auto &npc = server->addNPC("workerBee", {10, 15});

      WHEN("a player kills it") {
        client->sendMessage(CL_TARGET_ENTITY, makeArgs(npc.serial()));

        THEN("that player has 100 XP") { WAIT_UNTIL(user->xp() == 100); }
      }
    }

    AND_GIVEN("an elite NPC") {
      const auto &soldier = server->addNPC("soldierBee", {10, 15});

      THEN("it's worth 500XP") {
        CHECK(user->appropriateXPForKill(soldier) == 500);
      }
    }

    AND_GIVEN("a boss NPC") {
      const auto &queen = server->addNPC("queenBee", {10, 15});

      THEN("it's worth 1000XP") {
        CHECK(user->appropriateXPForKill(queen) == 1000);
      }
    }
  }
}

TEST_CASE_METHOD(TwoClientsWithData,
                 "Players can select stuff without interrupting their actions",
                 "[construction]") {
  GIVEN("towers type that takes time to construct") {
    useData(R"(
      <objectType
        id="tower" constructionTime="100" >
        <material id="wood" quantity="1" />
      </objectType>
      <item id="wood" />
    )");

    AND_GIVEN("Alice is constructing a tower") {
      cAlice->sendMessage(CL_CONSTRUCT, makeArgs("tower", 30, 30));

      WHEN("she selects (left-clicks) an existing tower") {
        const auto &existingTower = server->addObject("tower", {100, 100});
        cAlice->sendMessage(CL_SELECT_ENTITY, makeArgs(existingTower.serial()));

        THEN("the new tower still gets built") {
          WAIT_UNTIL(server->entities().size() == 2);
        }
      }

      WHEN("she selects (left-clicks) Bob") {
        cAlice->sendMessage(CL_SELECT_PLAYER, "Bob");

        THEN("the new tower still gets built") {
          WAIT_UNTIL_TIMEOUT(server->entities().size() == 1, 150);
        }
      }
    }
  }
}
