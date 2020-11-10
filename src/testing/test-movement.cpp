#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Thin objects block movement") {
  // Given a server and client;
  auto s = TestServer::WithData("thin_wall");
  auto c = TestClient::WithData("thin_wall");

  // And a wall just above the user
  s.addObject("wall", {10, 5});

  // When the user tries to move up, through the wall
  s.waitForUsers(1);
  REPEAT_FOR_MS(500) {
    c.sendMessage(CL_MOVE_TO, makeArgs(10, 3));
    SDL_Delay(5);
  }

  // He fails
  auto &user = s.getFirstUser();
  CHECK(user.location().y > 4);
}

TEST_CASE("Dead objects don't block movement") {
  // Given a server and client;
  auto s = TestServer::WithData("thin_wall");
  auto c = TestClient::WithData("thin_wall");

  // And a wall just above the user;
  s.addObject("wall", {10, 5});

  // And that wall is dead
  s.getFirstObject().reduceHealth(1000000);

  // When the user tries to move up, through the wall
  s.waitForUsers(1);
  auto &user = s.getFirstUser();
  REPEAT_FOR_MS(3000) {
    c.sendMessage(CL_MOVE_TO, makeArgs(10, 3));

    if (user.location().y < 3.5) break;
  }
  // He succeeds
  CHECK(user.location().y < 3.5);
  ;
}

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
      c.sendMessage(CL_MOVE_TO, makeArgs(10, 30));

      THEN("he gets past the NPC") {
        const auto &user = s.getFirstUser();
        WAIT_UNTIL(user.location().y > 20);
      }
    }
  }
}

TEST_CASE("Users walking through gates") {
  GIVEN("a gate object owned by Alice") {
    auto data = R"(
      <objectType id="gate" isGate="1" >
        <collisionRect x="-10" y="0" w="20" h="1" />
      </objectType>
    )";
    TestServer s = TestServer::WithDataString(data);
    s.addObject("gate", {10, 20}, "Alice");

    WHEN("Alice tries to move through it") {
      auto c = TestClient::WithUsernameAndDataString("Alice", data);
      s.waitForUsers(1);
      c.sendMessage(CL_MOVE_TO, makeArgs(10, 30));

      THEN("she gets past it") {
        const auto &alice = s.getFirstUser();
        WAIT_UNTIL(alice.location().y > 20);
      }
    }
    WHEN("Bob tries to move through it") {
      auto c = TestClient::WithUsernameAndDataString("Bob", data);
      s.waitForUsers(1);
      c.sendMessage(CL_MOVE_TO, makeArgs(10, 30));

      THEN("he doesn't get past it") {
        REPEAT_FOR_MS(1000);
        const auto &bob = s.getFirstUser();
        CHECK(bob.location().y < 20);
      }
    }
  }
}

TEST_CASE("Pets walking through gates") {
  GIVEN("a gate owned by Alice, and a dog") {
    auto data = R"(
      <objectType id="gate" isGate="1" >
        <collisionRect x="-10" y="0" w="20" h="1" />
      </objectType>
      <npcType id="dog" >
        <collisionRect x="0" y="0" w="1" h="1" />
      </npcType>
    )";
    TestServer s = TestServer::WithDataString(data);
    auto &pet = s.addNPC("dog", {10, 15});
    s.addObject("gate", {10, 20}, "Alice");

    WHEN("the dog tries to move through it") {
      REPEAT_FOR_MS(1000);
      pet.moveLegallyTowards({10, 30});

      THEN("it doesn't get past it") { CHECK(pet.location().y < 20); }
    }

    AND_GIVEN("the dog is owned by Alice") {
      pet.permissions.setPlayerOwner("Alice");

      WHEN("it tries to move through it") {
        REPEAT_FOR_MS(1000);
        pet.moveLegallyTowards({10, 30});

        THEN("it gets past it") { CHECK(pet.location().y > 20); }
      }
    }

    AND_GIVEN("the dog is owned by Bob") {
      pet.permissions.setPlayerOwner("Bob");

      WHEN("it tries to move through it") {
        REPEAT_FOR_MS(1000);
        pet.moveLegallyTowards({10, 30});

        THEN("it doesn't get past it") { CHECK(pet.location().y < 20); }
      }
    }
  }
}
