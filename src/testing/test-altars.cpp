#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("A user can't build multiple player-unique objects") {
  GIVEN("blonde and readhead objects with the \"wife\" player-unique tag") {
    auto data = R"(
      <item id="engagementRing" />
      <objectType id="blonde"
        constructionTime="1"
        playerUnique="wife" >
          <material id="engagementRing" />
      </objectType>
      <objectType id="redhead"
        constructionTime="1"
        playerUnique="wife" >
          <material id="engagementRing" />
      </objectType>
    )";
    auto s = TestServer::WithDataString(data);

    AND_GIVEN("Bob has a blonde wife") {
      s.addObject("blonde", {}, "Bob");

      SECTION("Bob can't have two wives") {
        WHEN("Bob logs in") {
          auto c = TestClient::WithUsernameAndData("Bob", "wives");
          s.waitForUsers(1);

          AND_WHEN("he tries to get a readhead wife") {
            c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 10, 15));

            THEN("he receives an error message") {
              c.waitForMessage(WARNING_UNIQUE_OBJECT);

              AND_THEN("there is still only one object in the world") {
                CHECK(s.entities().size() == 1);
              }
            }
          }
        }
      }

      SECTION("A different player can also have a wife") {
        WHEN("Charlie logs in") {
          auto c = TestClient::WithUsernameAndData("Charlie", "wives");
          s.waitForUsers(1);
          auto &user = s.getFirstUser();

          AND_WHEN("he tries to get a readhead wife") {
            c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 15, 15));
            REPEAT_FOR_MS(100);

            THEN("there are two objects (one each)") {
              CHECK(s.entities().size() == 2);
            }
          }
        }
      }

      SECTION("Bob can't give his wife to the city", "[city]") {
        AND_GIVEN("Bob is logged in") {
          auto c = TestClient::WithUsernameAndData("Bob", "wives");
          s.waitForUsers(1);

          AND_GIVEN("Bob is a citizen of Athens") {
            s.cities().createCity("Athens", {}, {});
            auto &bob = s.getFirstUser();
            s.cities().addPlayerToCity(bob, "Athens");

            WHEN("he tries to give his wife to Athens") {
              auto &wife = s.getFirstObject();
              c.sendMessage(CL_CEDE, makeArgs(wife.serial()));

              THEN("hereceives an error message") {
                c.waitForMessage(ERROR_CANNOT_CEDE);

                AND_THEN("the wife still belongs to him") {
                  CHECK(wife.permissions.isOwnedByPlayer("Bob"));
                }
              }
            }
          }
        }
      }

      SECTION("Dead wives can be replaced") {
        AND_GIVEN("Bob's wife is dead") {
          auto &firstWife = s.getFirstObject();
          firstWife.reduceHealth(firstWife.health());

          WHEN("he logs in") {
            auto c = TestClient::WithUsernameAndData("Bob", "wives");
            s.waitForUsers(1);

            AND_WHEN("he tries to get a readhead wife") {
              c.sendMessage(CL_CONSTRUCT, makeArgs("redhead", 10, 15));

              THEN("there are two wives") {
                WAIT_UNTIL(s.entities().size() == 2);
              }
            }
          }
        }
      }
    }
  }
}

TEST_CASE("Clients can discern player-uniqueness") {
  GIVEN("a player-unique house object") {
    auto data = R"(
      <objectType id="house" playerUnique="house" />
    )";
    auto c = TestClient::WithDataString(data);
    const auto &house = c.getFirstObjectType();

    THEN("clients know it's player-unique") {
      CHECK(house.isPlayerUnique());

      AND_THEN("it has client tags") { CHECK(house.hasTags()); }
    }
  }
}

TEST_CASE("End-of-tutorial altar") {
  GIVEN("an altar that ends the tutorial, and a user next to it") {
    auto data = R"(
      <objectType id="altar">
        <action target="endTutorial" />
      </objectType>
      <postTutorialSpawn x="20" y="20" />
    )";

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("altar", {10, 15});
    const auto &altar = s.getFirstObject();

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    auto oldLocation = user.location();
    const auto expectedLocation = MapPoint{20, 20};

    THEN("an altar can be added") { s.addObject("altar", {10, 15}); }

    WHEN("a user worships there") {
      c.sendMessage(CL_PERFORM_OBJECT_ACTION, makeArgs(altar.serial(), "_"s));
      REPEAT_FOR_MS(100);

      THEN("he is at the new specified location") {
        CHECK(user.location() != oldLocation);
        CHECK(user.location() == expectedLocation);
      }

      AND_WHEN("he dies") {
        user.reduceHealth(user.health());

        THEN("he respawns at the new location") {
          REPEAT_FOR_MS(100);
          CHECK(user.location() == expectedLocation);
        }
      }
    }
  }

  GIVEN("a different post-tutorial location, (30, 30)") {
    auto data = R"(
      <objectType id="altar">
        <action target="endTutorial" />
      </objectType>
      <postTutorialSpawn x="30" y="30" />
    )";

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("altar", {10, 15});
    const auto &altar = s.getFirstObject();

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    const auto expectedLocation = MapPoint{30, 30};

    WHEN("a user worships at the altar") {
      c.sendMessage(CL_PERFORM_OBJECT_ACTION, makeArgs(altar.serial(), "_"s));

      THEN("he is at (30, 30)") {
        WAIT_UNTIL(user.location() == expectedLocation);
      }
    }
  }

  GIVEN("a cost") {
    auto data = R"(
      <objectType id="altar">
        <action target="endTutorial" cost="coin" />
      </objectType>
      <item id="coin" />
    )";

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("altar", {10, 15});
    const auto &altar = s.getFirstObject();

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    WHEN("the user has the required item") {
      user.giveItem(&s.getFirstItem());

      AND_WHEN("he worships at the altar") {
        c.sendMessage(CL_PERFORM_OBJECT_ACTION, makeArgs(altar.serial(), "_"s));

        THEN("he the server survives") {
          REPEAT_FOR_MS(100);
          s.nop();
        }
      }
    }
  }

  GIVEN("the user owns an object") {
    auto data = R"(
      <objectType id="altar">
        <action target="endTutorial" />
      </objectType>
      <objectType id="house" />
      <item id="coin" />
    )";

    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    s.addObject("altar", {10, 15});
    const auto &altar = s.getFirstObject();

    s.waitForUsers(1);
    auto &user = s.getFirstUser();

    s.addObject("house", {5, 5}, user.name());

    WHEN("he worships at the altar") {
      c.sendMessage(CL_PERFORM_OBJECT_ACTION, makeArgs(altar.serial(), "_"s));

      THEN("he owns no objects") {
        REPEAT_FOR_MS(100);
        const auto &objectsOwnedByUser =
            s.objectsByOwner().getObjectsWithSpecificOwner(
                {Permissions::Owner::PLAYER, user.name()});
        CHECK(objectsOwnedByUser.size() == 0);
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Effect: teleport to area") {
  GIVEN("a slipgate that teleports the user near (200,200)") {
    useData(R"(
      <objectType id="slipgate">
        <action target="teleportToArea" d1="200" d2="200" d3="50" />
      </objectType>
      <objectType id="wall" >
        <collisionRect x="-5" y="-5" w="10" h="10" />
      </objectType>
    )");
    const auto &slipgate = server->addObject("slipgate", {5.0, 5.0});

    WHEN("a player users the slipgate") {
      client->sendMessage(CL_PERFORM_OBJECT_ACTION,
                          makeArgs(slipgate.serial(), "_"s));

      THEN("he is near (200,200)") {
        WAIT_UNTIL(distance(user->location(), MapPoint{200.0, 200.0}) <= 50.0);
      }
    }

    AND_GIVEN("there's an obstacle at exactly (200,200)") {
      server->addObject("wall", {200, 200});

      WHEN("a player users the slipgate") {
        client->sendMessage(CL_PERFORM_OBJECT_ACTION,
                            makeArgs(slipgate.serial(), "_"s));

        THEN("he moves somewhere other than (200,200)") {
          WAIT_UNTIL(distance(user->location(), MapPoint{200.0, 200.0}) <=
                     50.0);
          CHECK(user->location() != MapPoint{200.0, 200.0});
        }
      }
    }
  }
}
