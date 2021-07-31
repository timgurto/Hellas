#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Objects have no owner by default", "[permissions]") {
  // When a basic object is created
  TestServer s = TestServer::WithData("basic_rock");
  s.addObject("rock", {10, 10});
  WAIT_UNTIL(s.entities().size() == 1);

  // Then that object has no owner
  Object &rock = s.getFirstObject();
  CHECK_FALSE(rock.permissions.hasOwner());
}

TEST_CASE("Constructing an object grants ownership",
          "[permissions][construction]") {
  GIVEN("a logged-in client") {
    TestServer s = TestServer::WithData("brick_wall");
    TestClient c = TestClient::WithData("brick_wall");
    s.waitForUsers(1);

    WHEN("he constructs a wall") {
      c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 15));
      WAIT_UNTIL(s.entities().size() == 1);

      THEN("he is the wall's owner") {
        Object &wall = s.getFirstObject();
        CHECK(wall.permissions.hasOwner());
        CHECK(wall.permissions.isOwnedByPlayer(c->username()));
      }
    }
  }
}

TEST_CASE("Public-access objects", "[permissions][gathering]") {
  // Given a rock with no owner
  TestServer s = TestServer::WithData("basic_rock");
  TestClient c = TestClient::WithData("basic_rock");
  const auto &rock = s.addObject("rock", {10, 10});
  WAIT_UNTIL(c.objects().size() == 1);

  // When a user attempts to gather it
  c.sendMessage(CL_GATHER, makeArgs(rock.serial()));

  // Then he gathers, receives a rock item, and the rock object disapears
  User &user = s.getFirstUser();
  WAIT_UNTIL(user.action() == User::Action::GATHER);
  WAIT_UNTIL(user.action() == User::Action::NO_ACTION);
  WAIT_UNTIL_TIMEOUT(s.entities().empty(), 200);
  const Item &rockItem = s.getFirstItem();
  WAIT_UNTIL_TIMEOUT(user.inventory()[0].first.type() == &rockItem, 200);
}

TEST_CASE("The owner can access an owned object", "[permissions][gathering]") {
  // Given a rock owned by a user
  TestServer s = TestServer::WithData("basic_rock");
  TestClient c = TestClient::WithData("basic_rock");
  s.waitForUsers(1);
  User &user = s.getFirstUser();
  const auto &rock = s.addObject("rock", {10, 10}, user.name());
  WAIT_UNTIL(c.objects().size() == 1);

  // When he attempts to gather it
  c.sendMessage(CL_GATHER, makeArgs(rock.serial()));

  // Then he gathers, receives a rock item, and the object disapears
  WAIT_UNTIL(user.action() == User::Action::GATHER);
  WAIT_UNTIL(user.action() == User::Action::NO_ACTION);
  WAIT_UNTIL_TIMEOUT(s.entities().empty(), 200);
  const Item &rockItem = s.getFirstItem();
  WAIT_UNTIL_TIMEOUT(user.inventory()[0].first.type() == &rockItem, 200);
}

TEST_CASE("A non-owner cannot access an owned object",
          "[permissions][gathering]") {
  GIVEN("a rock owned by Alice") {
    TestServer s = TestServer::WithData("basic_rock");
    auto &rock = s.addObject("rock", {10, 10});
    rock.permissions.setPlayerOwner("Alice");

    WHEN("a different user attempts to gather it") {
      TestClient c = TestClient::WithData("basic_rock");
      WAIT_UNTIL(c.objects().size() == 1);
      c.sendMessage(CL_GATHER, makeArgs(rock.serial()));
      REPEAT_FOR_MS(500);

      THEN("the rock remains, and his inventory remains empty") {
        CHECK_FALSE(s.entities().empty());
        User &user = s.getFirstUser();
        CHECK_FALSE(user.inventory()[0].first.hasItem());
      }
    }
  }
}

TEST_CASE("A city can own an object", "[city][permissions]") {
  // Given a rock, and a city named Athens
  TestServer s = TestServer::WithData("basic_rock");
  s.cities().createCity("Athens", {}, {});
  s.addObject("rock", {10, 10});
  Object &rock = s.getFirstObject();

  // When its owner is set to Athens
  rock.permissions.setCityOwner("Athens");

  // Then an 'owner()' check matches the city of Athens;
  CHECK(rock.permissions.isOwnedByCity("Athens"));

  // And an 'owner()' check doesn't match a player named Athens
  CHECK_FALSE(rock.permissions.isOwnedByPlayer("Athens"));
}

TEST_CASE("City ownership is persistent", "[city][permissions][persistence]") {
  // Given a rock owned by Athens
  {
    TestServer s1 = TestServer::WithData("basic_rock");
    s1.cities().createCity("Athens", {}, {});
    s1.addObject("rock", {10, 10});
    Object &rock = s1.getFirstObject();
    rock.permissions.setCityOwner("Athens");
  }

  // When a new server starts up
  TestServer s2 = TestServer::WithDataAndKeepingOldData("basic_rock");

  // Then the rock is still owned by Athens
  Object &rock = s2.getFirstObject();
  CHECK(rock.permissions.isOwnedByCity("Athens"));
}

TEST_CASE("City members can use city objects",
          "[city][permissions][gathering]") {
  // Given a rock owned by Athens;
  TestServer s = TestServer::WithData("basic_rock");
  s.cities().createCity("Athens", {}, {});
  s.addObject("rock", {10, 10});
  Object &rock = s.getFirstObject();
  rock.permissions.setCityOwner("Athens");
  // And a client, who is a member of Athens
  TestClient c = TestClient::WithData("basic_rock");
  s.waitForUsers(1);
  User &user = s.getFirstUser();
  s.cities().addPlayerToCity(user, "Athens");

  // When he attempts to gather the rock
  WAIT_UNTIL(c.objects().size() == 1);
  c.sendMessage(CL_GATHER, makeArgs(rock.serial()));

  // Then he gathers;
  WAIT_UNTIL(user.action() == User::Action::GATHER);
  WAIT_UNTIL(user.action() == User::Action::NO_ACTION);
  // And he receives a Rock item;
  const Item &rockItem = s.getFirstItem();
  WAIT_UNTIL_TIMEOUT(user.inventory()[0].first.type() == &rockItem, 200);
  // And the Rock object disappears
  WAIT_UNTIL_TIMEOUT(s.entities().empty(), 200);
}

TEST_CASE("Non-members cannot use city objects",
          "[city][permissions][gathering]") {
  GIVEN("a rock owned by Athens") {
    TestServer s = TestServer::WithData("basic_rock");
    TestClient c = TestClient::WithData("basic_rock");

    s.cities().createCity("Athens", {}, {});
    s.addObject("rock", {10, 10});
    Object &rock = s.getFirstObject();
    rock.permissions.setCityOwner("Athens");

    s.waitForUsers(1);

    WHEN("a user attempts to gather the rock") {
      c.sendMessage(CL_GATHER, makeArgs(rock.serial()));
      REPEAT_FOR_MS(500);

      THEN("the rock remains") {
        CHECK_FALSE(s.entities().empty());

        AND_THEN("his inventory is empty") {
          User &user = s.getFirstUser();
          CHECK_FALSE(user.inventory()[0].first.hasItem());
        }
      }
    }
  }
}

TEST_CASE("Non-existent cities can't own objects", "[city][permissions]") {
  GIVEN("a rock, and no cities") {
    TestServer s = TestServer::WithData("basic_rock");
    s.addObject("rock", {10, 10});

    WHEN("the rock's owner is set to nonexistent city \"Athens\"") {
      Object &rock = s.getFirstObject();
      rock.permissions.setCityOwner("Athens");

      THEN("the rock has no owner") {
        CHECK_FALSE(rock.permissions.hasOwner());
      }
    }
  }
}

TEST_CASE("New objects are added to owner index", "[permissions]") {
  // Given a server with rock objects
  TestServer s = TestServer::WithData("basic_rock");

  // When a rock is added, owned by Alice
  s.addObject("rock", {}, "Alice");

  // The server's object-owner index knows about it
  Permissions::Owner owner(Permissions::Owner::PLAYER, "Alice");
  WAIT_UNTIL(s.objectsByOwner().getObjectsWithSpecificOwner(owner).size() == 1);
}

TEST_CASE("The object-owner index is initially empty", "[permissions]") {
  // Given an empty server
  TestServer s;

  // Then the object-owner index reports no objects belonging to Alice
  Permissions::Owner owner(Permissions::Owner::PLAYER, "Alice");
  CHECK(s.objectsByOwner().getObjectsWithSpecificOwner(owner).size() == 0);
}

TEST_CASE("A removed object is removed from the object-owner index",
          "[permissions]") {
  // Given a server
  TestServer s = TestServer::WithData("basic_rock");
  // And a rock object owned by Alice
  s.addObject("rock", {}, "Alice");

  // When the object is removed
  Object &rock = s.getFirstObject();
  s.removeEntity(rock);

  // Then the object-owner index reports no objects belonging to Alice
  Permissions::Owner owner(Permissions::Owner::PLAYER, "Alice");
  WAIT_UNTIL(s.objectsByOwner().getObjectsWithSpecificOwner(owner).size() == 0);
}

TEST_CASE("New ownership is reflected in the object-owner index",
          "[permissions]") {
  // Given a server with rock objects
  TestServer s = TestServer::WithData("basic_rock");

  // When a rock is added, owned by Alice;
  s.addObject("rock", {}, "Alice");

  // And the rock's ownership is changed to Bob;
  Object &rock = s.getFirstObject();
  rock.permissions.setPlayerOwner("Bob");

  // The server's object-owner index has it under Bob's name, not Alice's
  Permissions::Owner ownerAlice(Permissions::Owner::PLAYER, "Alice");
  WAIT_UNTIL(
      s.objectsByOwner().getObjectsWithSpecificOwner(ownerAlice).size() == 0);
  Permissions::Owner ownerBob(Permissions::Owner::PLAYER, "Bob");
  CHECK(s.objectsByOwner().getObjectsWithSpecificOwner(ownerBob).size() == 1);
}

TEST_CASE_METHOD(ServerAndClientWithData, "Objects can be given to citizens",
                 "[permissions][city][giving-objects]") {
  useData(R"(<objectType id="thing" />)");

  AND_GIVEN("the player is a citizen of Athens") {
    server->cities().createCity("Athens", {}, {});
    server->cities().addPlayerToCity(*user, "Athens");

    AND_GIVEN("Athens owns a thing") {
      auto &thing = server->addObject("thing");
      thing.permissions.setCityOwner("Athens");

      AND_GIVEN("the player is king of Athens") {
        (*server)->makePlayerAKing(*user);

        WHEN("the player tries to give it to himself") {
          client->sendMessage(CL_GIVE_OBJECT,
                              makeArgs(thing.serial(), user->name()));

          THEN("it is owned by him") {
            WAIT_UNTIL(thing.permissions.isOwnedByPlayer(user->name()));
          }
        }
      }
    }
  }
}

TEST_CASE("Non kings can't grant objects",
          "[permissions][city][giving-objects]") {
  // Given a Rock object type;
  auto s = TestServer::WithData("basic_rock");

  // And a city, Athens;
  s.cities().createCity("Athens", {}, {});

  // And its citizen, Alice;
  auto c = TestClient::WithUsernameAndData("Alice", "basic_rock");
  s.waitForUsers(1);
  auto &alice = s.getFirstUser();
  s.cities().addPlayerToCity(alice, "Athens");

  // And a rock, owned by Athens
  s.addObject("rock");
  auto &rock = s.getFirstObject();
  rock.permissions.setCityOwner("Athens");

  // When Alice tries to grant the rock to herself
  c.sendMessage(CL_GIVE_OBJECT, makeArgs(rock.serial(), "Alice"));

  // Then Alice receives an error message;
  c.waitForMessage(ERROR_NOT_A_KING);

  // And the rock is still owned by the city
  CHECK(rock.permissions.isOwnedByCity("Athens"));
}

TEST_CASE("Unowned objects cannot be granted",
          "[permissions][giving-objects]") {
  // Given a Rock object type;
  auto s = TestServer::WithData("basic_rock");

  // And a city, Athens;
  s.cities().createCity("Athens", {}, {});

  // And its king, Alice;
  auto c = TestClient::WithUsernameAndData("Alice", "basic_rock");
  s.waitForUsers(1);
  auto &alice = s.getFirstUser();
  s.cities().addPlayerToCity(alice, "Athens");
  s->makePlayerAKing(alice);

  // And an unowned rock
  s.addObject("rock");

  // When Alice tries to grant the rock to herself
  auto &rock = s.getFirstObject();
  c.sendMessage(CL_GIVE_OBJECT, makeArgs(rock.serial(), "Alice"));

  // Then Alice receives an error message;
  c.waitForMessage(WARNING_NO_PERMISSION);

  // And the rock is still unowned
  CHECK_FALSE(rock.permissions.hasOwner());
}

TEST_CASE("Only objects owned by your city can be granted",
          "[permissions][city][giving-objects]") {
  // Given a Rock object type;
  auto s = TestServer::WithData("basic_rock");

  // And two cities, Athens and Sparts;
  s.cities().createCity("Athens", {}, {});
  s.cities().createCity("Sparta", {}, {});

  // And Alice, Athens' king;
  auto c = TestClient::WithUsernameAndData("Alice", "basic_rock");
  s.waitForUsers(1);
  auto &alice = s.getFirstUser();
  s.cities().addPlayerToCity(alice, "Athens");
  s->makePlayerAKing(alice);

  // And a rock, owned by Sparta
  s.addObject("rock");
  auto &rock = s.getFirstObject();
  rock.permissions.setCityOwner("Sparta");

  // When Alice tries to grant the rock to herself
  c.sendMessage(CL_GIVE_OBJECT, makeArgs(rock.serial(), "Alice"));

  // Then Alice receives an error message;
  c.waitForMessage(WARNING_NO_PERMISSION);

  // And the rock is still owned by the city
  CHECK(rock.permissions.isOwnedByCity("Sparta"));
}

TEST_CASE("A player can gift his own objects",
          "[giving-objects][permissions]") {}

TEST_CASE("New object permissions are propagated to clients", "[permissions]") {
  GIVEN("an unowned Rock object") {
    auto s = TestServer::WithData("basic_rock");
    auto c = TestClient::WithData("basic_rock");

    s.addObject("rock");
    WAIT_UNTIL(c.objects().size() == 1);

    WHEN("the rock's owner is set to the user") {
      auto &rock = s.getFirstObject();
      rock.permissions.setPlayerOwner(c->username());

      THEN("he finds out") { WAIT_UNTIL(c.getFirstObject().belongsToPlayer()); }
    }
  }
}

TEST_CASE("No-access objects", "[permissions]") {
  GIVEN("an object marked no-access") {
    auto data = R"(
      <objectType id="house" />
    )";
    auto s = TestServer::WithDataString(data);

    auto noPermissions = Permissions::Owner{Permissions::Owner::NO_ACCESS, {}};
    const auto &house = s.addObject("house", {10, 15}, noPermissions);

    THEN("a user has no access") {
      CHECK_FALSE(house.permissions.doesUserHaveAccess("noname"));
    }
  }
}
