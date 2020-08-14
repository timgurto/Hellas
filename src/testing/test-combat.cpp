#include "../client/ClientNPC.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Players can attack immediately") {
  GIVEN("an NPC near a player") {
    auto data = R"(
      <npcType id="ant" maxHealth="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addNPC("ant", {10, 10});

    WHEN("the player attacks it") {
      const auto &ant = s.getFirstNPC();
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(ant.serial()));

      THEN("the NPC should be damaged very quickly") {
        WAIT_UNTIL_TIMEOUT(ant.health() < ant.stats().maxHealth, 100);
      }
    }
  }
}

TEST_CASE("Only belligerents can target each other") {
  GIVEN("Alice and Bob are at peace") {
    auto s = TestServer{};
    auto alice = TestClient::WithUsername("Alice");
    auto bob = TestClient::WithUsername("Bob");
    s.waitForUsers(2);
    User &uAlice = s.findUser("Alice"), &uBob = s.findUser("Bob");

    WHEN("Alice tries to target Bob") {
      alice.sendMessage(CL_TARGET_PLAYER, "Bob");

      THEN("She is not targeting him") {
        REPEAT_FOR_MS(100);
        CHECK_FALSE(uAlice.target() == &uBob);
      }
    }

    WHEN("Alice declares war on Bob") {
      alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");

      AND_WHEN("Alice tries to target Bob") {
        alice.sendMessage(CL_TARGET_PLAYER, "Bob");

        THEN("Alice is targeting Bob") { WAIT_UNTIL(uAlice.target() == &uBob); }
      }
    }
  }
}

TEST_CASE("Only belligerents can fight") {
  GIVEN("Alice and Bob are within melee range") {
    auto s = TestServer{};
    auto alice = TestClient::WithUsername("Alice");
    auto bob = TestClient::WithUsername("Bob");
    s.waitForUsers(2);

    User &uAlice = s.findUser("Alice"), &uBob = s.findUser("Bob");
    while (distance(uAlice.location(), uBob.location()) >
           Server::ACTION_DISTANCE)
      uAlice.moveLegallyTowards(uBob.location());

    WHEN("Alice tries to target Bob") {
      alice.sendMessage(CL_TARGET_PLAYER, "Bob");

      THEN("Bob doesn't lose health") {
        REPEAT_FOR_MS(500);
        CHECK(uBob.health() == uBob.stats().maxHealth);
      }
    }

    WHEN("Alice declares war on Bob") {
      alice.sendMessage(CL_DECLARE_WAR_ON_PLAYER, "Bob");

      AND_WHEN("Alice targets Bob") {
        alice.sendMessage(CL_TARGET_PLAYER, "Bob");

        THEN("Bob loses health") {
          WAIT_UNTIL(uBob.health() < uBob.stats().maxHealth);
        }
      }
    }
  }
}

TEST_CASE("Attack rate is respected", "[.flaky]") {
  // Given a server, with a wolf NPC which hits for 1 damage every 100ms;
  TestServer s = TestServer::WithData("wolf");
  s.addNPC("wolf", {10, 20});

  // And a nearby user
  TestClient c;
  s.waitForUsers(1);
  const User &user = s.getFirstUser();
  Hitpoints before = user.health();

  // When 1050ms elapse
  REPEAT_FOR_MS(1050);

  // Then the user has taken exactly 10 damage
  Hitpoints after = user.health();
  CHECK(after == before - 10);
}

TEST_CASE("Belligerents can attack each other's objects") {
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
  REQUIRE(distance(wolf.collisionRect(), user.collisionRect()) >
          Server::ACTION_DISTANCE);

  // When the client attempts to target the wolf
  WAIT_UNTIL(c.objects().size() == 1);
  ClientNPC &clientWolf = c.getFirstNPC();
  clientWolf.onRightClick();

  // Then his target is set to the wolf
  WAIT_UNTIL(user.target() == &wolf);
}

TEST_CASE("Clients receive nearby users' health values") {
  // Given a server and two clients, Alice and Bob;
  TestServer s;
  auto clientAlice = TestClient::WithUsername("Alice");
  auto clientBob = TestClient::WithUsername("Bob");

  // And Alice and Bob are at war
  s.wars().declare("Alice", "Bob");

  // When Alice is close to Bob;
  s.waitForUsers(2);
  const User &alice = s.findUser("Alice"), &bob = s.findUser("Bob");
  while (distance(alice.collisionRect(), bob.collisionRect()) >=
         Server::ACTION_DISTANCE) {
    clientAlice.sendMessage(CL_LOCATION,
                            makeArgs(bob.location().x, bob.location().y));
    SDL_Delay(5);
  }

  // And she attacks him
  clientAlice.sendMessage(CL_TARGET_PLAYER, "Bob");

  // Then Alice sees that Bob is damaged
  WAIT_UNTIL(clientAlice.otherUsers().size() == 1);
  const Avatar &localBob = clientAlice.getFirstOtherUser();
  WAIT_UNTIL(localBob.health() < localBob.maxHealth());
}

TEST_CASE("A player dying doesn't crash the server") {
  // Given a server and client;
  TestServer s;
  TestClient c;
  s.waitForUsers(1);

  // When the user dies
  User &user = s.getFirstUser();
  user.reduceHealth(999999);

  // The server survives
  s.nop();
}

TEST_CASE("Civilian NPCs") {
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

TEST_CASE("In-combat flag") {
  GIVEN("a user and a fox out of aggro range") {
    auto data = R"(
      <npcType id="fox" attack="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addNPC("fox", {100, 0});

    s.waitForUsers(1);
    auto &user = s.getFirstUser();
    auto &fox = s.getFirstNPC();

    THEN("The user is not in combat") { CHECK_FALSE(user.isInCombat()); }

    WHEN("the fox becomes aware of the user") {
      fox.makeAwareOf(user);
      WAIT_UNTIL(fox.target() == &user);
      THEN("the user is in combat") { WAIT_UNTIL(user.isInCombat()); }
    }
  }
}

TEST_CASE("Neutral NPCs") {
  GIVEN("a \"neutral\" NPC with attack") {
    auto data = R"(
      <npcType id="snake" attack="1" isNeutral="1" />
    )";
    auto s = TestServer::WithDataString(data);

    s.addNPC("snake", {15, 10});

    WHEN("a user gets close") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      auto healthBefore = user.health();

      THEN("he doesn't get attacked") {
        REPEAT_FOR_MS(100) REQUIRE(user.health() == healthBefore);
      }
    }
  }
}

TEST_CASE("Targeting civilians after attacking") {
  GIVEN("A user between an attackable sloth and a civilian maiden") {
    auto data = R"(
      <newPlayerSpawn x="10" y="10" range="0" />
      <npcType id="sloth" maxHealth="1000" />
      <npcType id="maiden" maxHealth="1000" isCivilian="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    const auto &sloth = s.addNPC("sloth", {10, 15});
    const auto &maiden = s.addNPC("maiden", {10, 5});

    s.waitForUsers(1);

    WHEN("he attacks the sloth") {
      c.sendMessage(CL_TARGET_ENTITY, makeArgs(sloth.serial()));
      REPEAT_FOR_MS(100);

      AND_WHEN("he right-clicks the maiden") {
        auto cMaiden = c.objects()[maiden.serial()];
        cMaiden->onRightClick();

        THEN("the maiden doesn't take damage") {
          REPEAT_FOR_MS(3000);
          CHECK(maiden.health() == maiden.stats().maxHealth);
        }
      }
    }
  }
}

TEST_CASE("XP from kills") {
  GIVEN("normal and elite NPC types") {
    auto data = R"(
      <npcType id="queenBee" maxHealth="1" elite="1" />
      <npcType id="workerBee" maxHealth="1" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    AND_GIVEN("a normal NPC") {
      const auto &npc = s.addNPC("workerBee", {10, 15});

      WHEN("a player kills it") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(npc.serial()));

        THEN("that player has 100 XP") {
          const auto &user = s.getFirstUser();
          WAIT_UNTIL(user.xp() == 100);
        }
      }
    }

    AND_GIVEN("an elite NPC") {
      const auto &elite = s.addNPC("queenBee", {10, 15});

      WHEN("a player kills it") {
        c.sendMessage(CL_TARGET_ENTITY, makeArgs(elite.serial()));

        THEN("that player has 400 XP") {
          const auto &user = s.getFirstUser();
          WAIT_UNTIL(user.xp() == 400);
        }
      }
    }
  }
}
