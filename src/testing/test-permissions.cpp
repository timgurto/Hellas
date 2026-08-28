#include "TestClient.h"
#include "TestFixtures.h"
#include "testing.h"
#include "TestServer.h"

TEST_CASE("Objects have no owner by default", "[permissions]") {
  // When a basic object is created
  auto data = R"(
    <item id="stone" />
    <objectType id="rock" canGather="1" gatherTime="100" >
      <yield id="stone" initialMean="1" initialSD="0" />
    </objectType>
  )";
  TestServer s = TestServer::WithDataString(data);
  s.addObject("rock", {10, 10});
  WAIT_UNTIL(s.entities().size() == 1);

  // Then that object has no owner
  Object &rock = s.getFirstObject();
  CHECK_FALSE(rock.permissions.hasOwner());
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Constructing an object grants ownership",
                 "[permissions][construction]") {
  useData(R"(
    <item id="brick" />
    <objectType id="wall" constructionTime="0" >
      <material id="brick" quantity="1" />
    </objectType>
  )");

  WHEN("a user constructs a wall") {
    client->sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 15));
    WAIT_UNTIL(server->entities().size() == 1);

    THEN("he is the wall's owner") {
      Object &wall = server->getFirstObject();
      CHECK(wall.permissions.hasOwner());
      CHECK(wall.permissions.isOwnedByPlayer(client->name()));
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Public-access objects",
                 "[permissions][gathering]") {
  useData(R"(
    <item id="stone" />
    <objectType id="rock" canGather="1" gatherTime="100" >
      <yield id="stone" initialMean="1" initialSD="0" />
    </objectType>
  )");

  // Given a rock with no owner
  const auto &rock = server->addObject("rock", {10, 10});
  WAIT_UNTIL(client->objects().size() == 1);

  // When a user attempts to gather it
  client->sendMessage(CL_GATHER, makeArgs(rock.serial()));

  // Then he gathers, receives a rock item, and the rock object disapears
  WAIT_UNTIL((user->action() == User::Action::GATHER));
  WAIT_UNTIL((user->action() == User::Action::NO_ACTION));
  WAIT_UNTIL_TIMEOUT(server->entities().empty(), 200);
  const Item &rockItem = server->getFirstItem();
  WAIT_UNTIL_TIMEOUT(user->inventory()[0].type() == &rockItem, 200);
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "The owner can access an owned object",
                 "[permissions][gathering]") {
  useData(R"(
    <item id="stone" />
    <objectType id="rock" canGather="1" gatherTime="100" >
      <yield id="stone" initialMean="1" initialSD="0" />
    </objectType>
  )");

  // Given a rock owned by the user
  const auto &rock = server->addObject("rock", {10, 10}, user->name());
  WAIT_UNTIL(client->objects().size() == 1);

  // When he attempts to gather it
  client->sendMessage(CL_GATHER, makeArgs(rock.serial()));

  // Then he gathers, receives a rock item, and the object disapears
  WAIT_UNTIL((user->action() == User::Action::GATHER));
  WAIT_UNTIL((user->action() == User::Action::NO_ACTION));
  WAIT_UNTIL_TIMEOUT(server->entities().empty(), 200);
  const Item &rockItem = server->getFirstItem();
  WAIT_UNTIL_TIMEOUT(user->inventory()[0].type() == &rockItem, 200);
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "A non-owner cannot access an owned object",
                 "[permissions][gathering]") {
  GIVEN("a rock owned by Alice") {
    useData(R"(
      <item id="stone" />
      <objectType id="rock" canGather="1" gatherTime="100" >
        <yield id="stone" initialMean="1" initialSD="0" />
      </objectType>
    )");
    auto &rock = server->addObject("rock", {10, 10});
    rock.permissions.setPlayerOwner("Alice");

    WHEN("a different user attempts to gather it") {
      WAIT_UNTIL(client->objects().size() == 1);
      client->sendMessage(CL_GATHER, makeArgs(rock.serial()));
      REPEAT_FOR_MS(500);

      THEN("the rock remains, and his inventory remains empty") {
        CHECK_FALSE(server->entities().empty());
        CHECK_FALSE(user->inventory()[0].hasItem());
      }
    }
  }
}

TEST_CASE("A city can own an object", "[city][permissions]") {
  // Given a rock, and a city named Athens
  auto data = R"(
    <objectType id="rock" />
  )";
  TestServer s = TestServer::WithDataString(data);
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
  auto data = R"(
    <objectType id="rock" />
  )";
  {
    TestServer s1 = TestServer::WithDataString(data);
    s1.cities().createCity("Athens", {}, {});
    s1.addObject("rock", {10, 10});
    Object &rock = s1.getFirstObject();
    rock.permissions.setCityOwner("Athens");
  }

  // When a new server starts up
  TestServer s2 = TestServer::WithDataStringAndKeepingOldData(data);

  // Then the rock is still owned by Athens
  Object &rock = s2.getFirstObject();
  CHECK(rock.permissions.isOwnedByCity("Athens"));
}

TEST_CASE_METHOD(ServerAndClientWithData, "City members can use city objects",
                 "[city][permissions][gathering]") {
  useData(R"(
    <item id="stone" />
    <objectType id="rock" canGather="1" gatherTime="100" >
      <yield id="stone" initialMean="1" initialSD="0" />
    </objectType>
  )");

  // Given a rock owned by Athens
  server->cities().createCity("Athens", {}, {});
  server->addObject("rock", {10, 10});
  Object &rock = server->getFirstObject();
  rock.permissions.setCityOwner("Athens");

  // And the user is a member of Athens
  server->cities().addPlayerToCity(*user, "Athens");

  // When he attempts to gather the rock
  WAIT_UNTIL(client->objects().size() == 1);
  client->sendMessage(CL_GATHER, makeArgs(rock.serial()));

  // Then he gathers;
  WAIT_UNTIL((user->action() == User::Action::GATHER));
  WAIT_UNTIL((user->action() == User::Action::NO_ACTION));
  // And he receives a Rock item;
  const Item &rockItem = server->getFirstItem();
  WAIT_UNTIL_TIMEOUT(user->inventory()[0].type() == &rockItem, 200);
  // And the Rock object disappears
  WAIT_UNTIL_TIMEOUT(server->entities().empty(), 200);
}

TEST_CASE_METHOD(ServerAndClientWithData, "Non-members cannot use city objects",
                 "[city][permissions][gathering]") {
  GIVEN("a rock owned by Athens") {
    useData(R"(
      <item id="stone" />
      <objectType id="rock" canGather="1" gatherTime="100" >
        <yield id="stone" initialMean="1" initialSD="0" />
      </objectType>
    )");

    server->cities().createCity("Athens", {}, {});
    server->addObject("rock", {10, 10});
    Object &rock = server->getFirstObject();
    rock.permissions.setCityOwner("Athens");

    WHEN("a user attempts to gather the rock") {
      client->sendMessage(CL_GATHER, makeArgs(rock.serial()));
      REPEAT_FOR_MS(500);

      THEN("the rock remains") {
        CHECK_FALSE(server->entities().empty());

        AND_THEN("his inventory is empty") {
          CHECK_FALSE(user->inventory()[0].hasItem());
        }
      }
    }
  }
}

TEST_CASE("Non-existent cities can't own objects", "[city][permissions]") {
  GIVEN("a rock, and no cities") {
    TestServer s = TestServer::WithDataString(R"(
      <objectType id="rock" />
    )");
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
  TestServer s = TestServer::WithDataString(R"(
    <objectType id="rock" />
  )");

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
  TestServer s = TestServer::WithDataString(R"(
    <objectType id="rock" />
  )");
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
  TestServer s = TestServer::WithDataString(R"(
    <objectType id="rock" />
  )");

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

TEST_CASE_METHOD(TwoClientsWithData, "Giving objects", "[permissions][city]") {
  useData(R"(<objectType id="thing" />)");

  SECTION("Standard user -> user") {
    GIVEN("Alice owns a thing") {
      const auto &thing = server->addObject("thing", {}, "Alice");

      WHEN("she gives it to Bob") {
        cAlice->sendMessage(CL_GIVE_OBJECT, makeArgs(thing.serial(), "Bob"));

        THEN("it is owned by Bob") {
          WAIT_UNTIL(thing.permissions.isOwnedByPlayer("Bob"));
        }
      }

      SECTION("Bad recipient name") {
        WHEN("she tries to give it to a nonexistent player") {
          cAlice->sendMessage(CL_GIVE_OBJECT,
                              makeArgs(thing.serial(), "Notarealplayer"));

          THEN("it is still owned by her") {
            REPEAT_FOR_MS(100);
            CHECK(thing.permissions.isOwnedByPlayer("Alice"));
          }

          THEN("she receives an error message") {
            CHECK(cAlice->waitForMessage(ERROR_USER_NOT_FOUND));
          }
        }
      }
    }
  }

  SECTION("Unowned objects can't be given") {
    GIVEN("an unowned thing") {
      auto &thing = server->addObject("thing");

      WHEN("Alice tries to give it to herself") {
        cAlice->sendMessage(CL_GIVE_OBJECT, makeArgs(thing.serial(), "Alice"));

        THEN("she receives a warning message") {
          CHECK(cAlice->waitForMessage(WARNING_NO_PERMISSION));
        }

        THEN("it's still unowned") {
          REPEAT_FOR_MS(100);
          CHECK_FALSE(thing.permissions.hasOwner());
        }
      }
    }
  }

  SECTION("Kings can give city objects") {
    GIVEN("Alice is a citizen of Athens") {
      server->cities().createCity("Athens", {}, {});
      server->cities().addPlayerToCity(*uAlice, "Athens");

      AND_GIVEN("Athens owns a thing") {
        auto &thing = server->addObject("thing");
        thing.permissions.setCityOwner("Athens");

        AND_GIVEN("Alice is king of Athens") {
          (*server)->makePlayerAKing(*uAlice);

          WHEN("she tries to give it to herself") {
            cAlice->sendMessage(CL_GIVE_OBJECT,
                                makeArgs(thing.serial(), "Alice"));

            THEN("it is owned by her") {
              WAIT_UNTIL(thing.permissions.isOwnedByPlayer("Alice"));
            }
          }
        }

        SECTION("Non-kings can't give city objects") {
          WHEN("Alice tries to give it to herself") {
            cAlice->sendMessage(CL_GIVE_OBJECT,
                                makeArgs(thing.serial(), "Alice"));

            THEN("she receives an error message") {
              CHECK(cAlice->waitForMessage(WARNING_NO_PERMISSION));
            }

            THEN("it's still owned by the city") {
              REPEAT_FOR_MS(100);
              CHECK(thing.permissions.isOwnedByCity("Athens"));
            }
          }
        }
      }

      SECTION("A king can't give another city's object") {
        AND_GIVEN("Sparta owns a thing") {
          server->cities().createCity("Sparta", {}, {});
          auto &thing = server->addObject("thing");
          thing.permissions.setCityOwner("Sparta");

          AND_GIVEN("Alice is king of Athens") {
            (*server)->makePlayerAKing(*uAlice);

            WHEN("she tries to give it to himself") {
              cAlice->sendMessage(CL_GIVE_OBJECT,
                                  makeArgs(thing.serial(), "Alice"));

              THEN("he receives a warning message") {
                CHECK(cAlice->waitForMessage(WARNING_NO_PERMISSION));
              }

              THEN("it's still owned by Sparta") {
                REPEAT_FOR_MS(100);
                CHECK(thing.permissions.isOwnedByCity("Sparta"));
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "New object permissions are propagated to clients",
                 "[permissions]") {
  GIVEN("an unowned Rock object") {
    useData(R"(
      <item id="stone" />
      <objectType id="rock" canGather="1" gatherTime="100" >
        <yield id="stone" initialMean="1" initialSD="0" />
      </objectType>
    )");

    server->addObject("rock");
    WAIT_UNTIL(client->objects().size() == 1);

    WHEN("the rock's owner is set to the user") {
      auto &rock = server->getFirstObject();
      rock.permissions.setPlayerOwner(client->name());

      THEN("he finds out") {
        WAIT_UNTIL(client->getFirstObject().belongsToPlayer());
      }
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
      CHECK_FALSE(house.permissions.canUserAccessContainer("noname"));
    }
  }
}

TEST_CASE_METHOD(TwoClientsWithData, "New owners can see container contents",
                 "[permissions][containers]") {
  GIVEN("a box") {
    useData(R"(
      <objectType id="box" >
        <container slots="1" />
      </objectType>
      <item id="fruit" />
    )");
    const auto *fruit = &server->getFirstItem();
    auto &box = server->addObject("box", {10, 10});
    WAIT_UNTIL(cBob->objects().size() == 1);
    WAIT_UNTIL(cAlice->objects().size() == 1);
    const auto &boxInBobsClient = cBob->getFirstObject();
    const auto &boxInAlicesClient = cAlice->getFirstObject();

    AND_GIVEN("Alice owns the box") {
      box.permissions.setPlayerOwner("Alice");

      AND_GIVEN("the box contains fruit") {
        box.container().addItems(fruit);

        AND_GIVEN("Alice and Bob are in a city") {
          server->cities().createCity("Athens", {}, {});
          server->cities().addPlayerToCity(*uAlice, "Athens");
          server->cities().addPlayerToCity(*uBob, "Athens");

          WHEN("Alice cedes the box to the city") {
            cAlice->sendMessage(CL_CEDE, makeArgs(box.serial()));

            THEN("Bob knows it contains fruit") {
              WAIT_UNTIL(boxInBobsClient.container()[0].first.type());
            }
          }
        }

        WHEN("Alice gives the box to Bob") {
          cAlice->sendMessage(CL_GIVE_OBJECT, makeArgs(box.serial(), "Bob"));

          THEN("he knows it contains fruit") {
            WAIT_UNTIL(boxInBobsClient.container()[0].first.type());
          }
        }

        WHEN("Alice gives the box to a logged-out player") {
          {
            auto cCharlie = TestClient::WithUsername("Charlie");
            WAIT_UNTIL((*server)->doesPlayerExist("Charlie"));
          }
          cAlice->sendMessage(CL_GIVE_OBJECT,
                              makeArgs(box.serial(), "Charlie"));

          THEN("the server doesn't crash") { server->nop(); }
        }
      }
    }

    AND_GIVEN("a city owns the box") {
      server->cities().createCity("Athens", {}, {});
      box.permissions.setCityOwner("Athens");

      AND_GIVEN("the box contains fruit") {
        box.container().addItems(fruit);

        WHEN("Alice joins the city") {
          server->cities().addPlayerToCity(*uAlice, "Athens");

          THEN("she knows it contains fruit") {
            WAIT_UNTIL(boxInAlicesClient.container()[0].first.type());
          }
        }
      }
    }
  }
}
