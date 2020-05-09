#include "RemoteClient.h"
#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("On login, players are told about their distant objects",
          "[.flaky][culling]") {
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

TEST_CASE("On login, players are not told about others' distant objects",
          "[.flaky][culling]") {
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
          "[.slow][culling][remote]") {
  // Given a server with a large map;
  TestServer s = TestServer::WithData("signpost");

  // And a client at (10, 10);
  TestClient c = TestClient::WithData("signpost");
  s.waitForUsers(1);

  // And a client at (1000, 10);
  User::newPlayerSpawn = {1000, 10};  // TODO pop this state after test
  User::spawnRadius = 0;
  RemoteClient rc("-data testing/data/signpost");
  s.waitForUsers(2);
  REPEAT_FOR_MS(500);
  CHECK(c.otherUsers().size() == 0);

  // When the first moves within range of the second
  auto startTime = SDL_GetTicks();
  while (c->character().location().x < 900) {
    if (SDL_GetTicks() - startTime > 100000) break;

    c.sendMessage(CL_LOCATION, makeArgs(1000, 10));

    // Then he becomes aware of him
    if (c.otherUsers().size() == 1) break;
    SDL_Delay(5);
  }
  CHECK(c.otherUsers().size() == 1);
}

TEST_CASE("When a player moves away from his object, he is still aware of it",
          "[.slow][culling]") {
  // Given a server with signpost objects;
  TestServer s = TestServer::WithData("signpost");

  // And a signpost near the user spawn point that belongs to Alice;
  s.addObject("signpost", {10, 15}, "Alice");

  // And Alice is logged in
  TestClient c = TestClient::WithUsernameAndData("Alice", "signpost");
  s.waitForUsers(1);
  WAIT_UNTIL(c.objects().size() == 1);

  // When Alice moves out of range of the signpost
  auto startTime = SDL_GetTicks();
  while (c->character().location().x < 1000) {
    if (SDL_GetTicks() - startTime > 100000) break;

    c.sendMessage(CL_LOCATION, makeArgs(1010, 10));

    // Then she is still aware of it
    if (c.objects().size() == 0) break;
    SDL_Delay(5);
  }
  CHECK(c.objects().size() == 1);
}

TEST_CASE(
    "When a player moves away from his city's object, he is still aware of it",
    "[.slow][culling]") {
  // Given a server with signpost objects;
  TestServer s = TestServer::WithData("signpost");

  // And a city named Athens
  s.cities().createCity("Athens", {}, {});

  // And a signpost near the user spawn point that belongs to Athens;
  s.addObject("signpost", {10, 15});
  Object &signpost = s.getFirstObject();
  signpost.permissions.setCityOwner("Athens");

  // And Alice is logged in
  TestClient c = TestClient::WithUsernameAndData("Alice", "signpost");

  // And Alice is a member of Athens
  s.waitForUsers(1);
  User &user = s.getFirstUser();
  s.cities().addPlayerToCity(user, "Athens");

  // When Alice moves out of range of the signpost
  WAIT_UNTIL(c.objects().size() == 1);
  auto startTime = SDL_GetTicks();
  while (c->character().location().x < 1000) {
    if (SDL_GetTicks() - startTime > 100000) break;

    c.sendMessage(CL_LOCATION, makeArgs(1010, 10));

    // Then she is still aware of it
    if (c.objects().size() == 0) break;
    SDL_Delay(5);
  }
  CHECK(c.objects().size() == 1);
}

TEST_CASE("New citizens find out about city objects") {
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
          clientNPC.onRightClick(c.client());
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

TEST_CASE("Out-of-range objects are forgotten", "[.slow][only]") {
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
    c.sendMessage(CL_LOCATION, makeArgs(1010, 10));

    // Then he is no longer aware of it
    if (c.objects().size() == 0) break;
    SDL_Delay(5);
  }
  CHECK(c.objects().size() == 0);
}
