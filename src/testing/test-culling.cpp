#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Connecting players are told about their distant objects",
          "[.flaky][persistence][permissions]") {
  // Given an object at (10000,10000) owned by Alice
  TestServer s = TestServer::WithData("signpost");
  s.addObject("signpost", {10000, 10000}, "Alice");

  // When Alice logs in
  TestClient c = TestClient::WithUsernameAndData("Alice", "signpost");
  WAIT_UNTIL_TIMEOUT(s.users().size() == 1, 10000);
  s.waitForUsers(1);

  // Alice knows about the object
  REPEAT_FOR_MS(500);
  CHECK(c.objects().size() == 1);
}
TEST_CASE("Connecting players are told about their distant pets") {
  GIVEN("Alice has a pet armadillo very far away") {
    auto s = TestServer::WithData("armadillos");
    auto &armadillo = s.addNPC("armadillo", {10000, 10000});
    armadillo.permissions.setPlayerOwner("Alice");

    WHEN("she logs in") {
      auto c = TestClient::WithUsernameAndData("Alice", "armadillos");

      THEN("she knows about her pet") { WAIT_UNTIL(c.objects().size() == 1); }
    }
  }
}

TEST_CASE("Connecting players are not told about others' distant objects",
          "[.flaky][persistence][permissions]") {
  // Given an object at (10000,10000) owned by Alice
  TestServer s = TestServer::WithData("signpost");
  s.addObject("signpost", {10000, 10000}, "Bob");

  // When Alice logs in
  TestClient c = TestClient::WithUsernameAndData("Alice", "signpost");
  WAIT_UNTIL_TIMEOUT(s.users().size() == 1, 10000);
  s.waitForUsers(1);

  // Alice does not know about the object
  REPEAT_FOR_MS(500);
  CHECK(c.objects().empty());
}

TEST_CASE("When one user approaches another, he finds out about him",
          "[.slow]") {
  // Given a server with a large map;
  auto s = TestServer::WithData("signpost");

  // And Alice is at (10, 10);
  auto alice = TestClient::WithUsernameAndData("Alice", "signpost");
  s.waitForUsers(1);

  // And Bob is at (1000, 10);
  User::newPlayerSpawn = {1000, 10};  // TODO pop this state after test
  User::spawnRadius = 0;
  auto bob = TestClient::WithUsernameAndData("Bob", "signpost");
  s.waitForUsers(2);
  REPEAT_FOR_MS(500);
  CHECK(alice.otherUsers().size() == 0);

  // When Alice moves within range of Bob
  auto startTime = SDL_GetTicks();
  while (alice->character().location().x < 900) {
    if (SDL_GetTicks() - startTime > 100000) break;

    alice.sendMessage(CL_MOVE_TO, makeArgs(1000, 10));

    // Then Alice becomes aware of Bob
    if (alice.otherUsers().size() == 1) break;
    SDL_Delay(5);
  }
  CHECK(alice.otherUsers().size() == 1);
}

TEST_CASE_METHOD(ServerAndClientWithDataFiles,
                 "When a player moves away from his [city's] object, he is "
                 "still aware of it",
                 "[city][permissions]") {
  useData("signpost");

  GIVEN("the player is in a city") {
    server->cities().createCity("Athens", {}, {});
    server->cities().addPlayerToCity(*user, "Athens");

    AND_GIVEN("signposts near the user that belong to himself and his city") {
      auto &personalSignpost = server->addObject("signpost", {10, 15});
      auto &citySignpost = server->addObject("signpost", {10, 5});
      personalSignpost.permissions.setPlayerOwner(user->name());
      citySignpost.permissions.setCityOwner("Athens");

      AND_GIVEN("he is aware of them") {
        WAIT_UNTIL(client->objects().size() == 2);

        WHEN("he moves out of range of the signposts") {
          const auto startTime = SDL_GetTicks();
          while (user->location().x < 1000) {
            if (SDL_GetTicks() - startTime > 100000) {
              break;
            }  // 100s timeout
            if (client->objects().size() < 2) {
              break;
            }  // fail early

            client->sendMessage(CL_MOVE_TO, makeArgs(1010, 10));
            SDL_Delay(5);
          }

          THEN("he is still aware of them") {
            REPEAT_FOR_MS(100);
            CHECK(client->objects().size() == 2);
          }
        }
      }
    }
  }
}

TEST_CASE("New citizens find out about city objects", "[city][permissions]") {
  GIVEN("a city object far away from a player") {
    auto s = TestServer::WithData("signpost");
    auto c = TestClient::WithData("signpost");

    s.cities().createCity("Athens", {}, {});
    s.addObject("signpost", {1000, 1000}, {Permissions::Owner::CITY, "Athens"});

    THEN("the player doesn't know about the object") {
      s.waitForUsers(1);
      REPEAT_FOR_MS(100);
      CHECK(c.objects().empty());

      AND_WHEN("the player joins the city") {
        s.cities().addPlayerToCity(s.getFirstUser(), "Athens");

        THEN("he knows about the object") {
          WAIT_UNTIL(c.objects().size() == 1);
        }
      }
    }
  }
}

TEST_CASE("Unwatching NPCs", "[.flaky]") {
  GIVEN("an NPC with a window") {
    auto data = R"(
      <npcType id="questgiver" />
      <quest id="quest1" startsAt="questgiver" endsAt="questgiver" />
    )";
    auto s = TestServer::WithDataString(data);
    s.addNPC("questgiver", {10, 15});

    WHEN("a user logs in") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);

      AND_WHEN("he finds out about the NPC") {
        WAIT_UNTIL(c.objects().size() == 1);
        auto &clientNPC = c.getFirstNPC();

        AND_WHEN("he opens the window") {
          clientNPC.onRightClick();
          CHECK(clientNPC.window());

          AND_WHEN("he moves away from the NPC") {
            auto &user = s.getFirstUser();
            user.teleportTo({200, 200});

            THEN("there is no error message") {
              CHECK_FALSE(c.waitForMessage(WARNING_DOESNT_EXIST));
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Out-of-range objects are forgotten", "[.slow]") {
  // Given a server and client with signpost objects;
  TestServer s = TestServer::WithData("signpost");
  TestClient c = TestClient::WithData("signpost");

  // And a signpost near the user spawn
  s.addObject("signpost", {10, 15});

  // And the client is aware of it
  s.waitForUsers(1);
  WAIT_UNTIL(c.objects().size() == 1);

  // When the client moves out of range of the signpost
  while (c->character().location().x < 1000) {
    c.sendMessage(CL_MOVE_TO, makeArgs(1010, 10));

    // Then he is no longer aware of it
    if (c.objects().size() == 0) break;
    SDL_Delay(5);
  }
  CHECK(c.objects().size() == 0);
}
