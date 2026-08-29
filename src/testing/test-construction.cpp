#include "TestClient.h"
#include "TestFixtures.h"
#include "testing.h"
#include "TestServer.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Construction materials can be added",
                 "[construction]") {
  GIVEN("a wall that requires a brick") {
    useData(R"(
      <objectType
        id="wall" constructionTime="0" >
        <material id="brick" quantity="1" />
      </objectType>
      <item id="brick" />
    )");

    AND_GIVEN("a user has a brick") {
      user->giveItem(&*server->items().begin());

      WHEN("he starts building a wall") {
        client->sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 15));
        WAIT_UNTIL(server->entities().size() == 1);
        const Object &wall = server->getFirstObject();

        THEN("it needs materials") { CHECK(wall.isBeingBuilt()); }

        AND_WHEN("he gives adds a brick") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                      wall.serial(), 0));

          THEN("construction has finished") {
            WAIT_UNTIL(!wall.isBeingBuilt());
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Client knows about default constructions", "[construction]") {
  GIVEN("a wall that requires a brick, and has no unlocks specified") {
    useData(R"(
      <objectType
        id="wall" constructionTime="0" >
        <material id="brick" quantity="1" />
      </objectType>
      <item id="brick" />
    )");

    THEN("a user knows how to build one") {
      CHECK(client->knowsConstruction("wall"));
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "New client doesn't know any locked constructions",
                 "[construction]") {
  GIVEN("a constructable table with an unlock") {
    useData(R"(
      <objectType
        id="table" constructionTime="0" >
        <material id="wood" quantity="5" />
        <unlockedBy item="wood" />
      </objectType>
      <item id="wood" />
    )");

    THEN("a new user doesn't know it") {
      CHECK_FALSE(client->knowsConstruction("table"));
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Unique objects are unique",
                 "[construction]") {
  GIVEN("a buildable, unique object") {
    useData(R"(
      <objectType id="throne" constructionTime="0" isUnique="1" >
        <material id="gold" quantity="1" />
      </objectType>
      <item id="gold" />
    )");

    WHEN("a user builds one") {
      client->sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 15));
      WAIT_UNTIL(server->entities().size() == 1);

      AND_WHEN("he tries to build another one") {
        client->sendMessage(CL_CONSTRUCT, makeArgs("throne", 15, 10));

        THEN("he gets a warning") {
          bool isConstructionRejected =
              client->waitForMessage(WARNING_UNIQUE_OBJECT);
          CHECK(isConstructionRejected);

          AND_THEN("there is still only the one object") {
            CHECK(server->entities().size() == 1);
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient,
                 "Constructing invalid object fails gracefully",
                 "[construction]") {
  WHEN("a user tries to build a nonexistent object") {
    client.sendMessage(CL_CONSTRUCT, makeArgs("notARealObject", 10, 15));

    THEN("the server survives") {
      REPEAT_FOR_MS(100);
      server.nop();
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Objects can be unbuildable",
                 "[construction]") {
  GIVEN("an object with materials, but marked as \"unbuildable\"") {
    useData(R"(
    <objectType id="treehouse" constructionTime="0" isUnbuildable="1" >
      <material id="wood" quantity="10" />
    </objectType>
    <item id="wood" />
    )");

    WHEN("a user tries to build one") {
      client->sendMessage(CL_CONSTRUCT, makeArgs("treehouse", 10, 15));
      REPEAT_FOR_MS(100);

      THEN("there are still no objects") {
        CHECK(server->entities().size() == 0);
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Clients can't see unbuildable constructions",
                 "[construction]") {
  GIVEN("an object with materials, but marked as \"unbuildable\"") {
    useData(R"(
    <objectType id="treehouse" constructionTime="0" isUnbuildable="1" >
      <material id="wood" quantity="10" />
    </objectType>
    <item id="wood" />
    )");

    THEN("a user can't see it as a construction option") {
      CHECK_FALSE(client->knowsConstruction("treehouse"));
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Object types without materials can't be built",
                 "[construction]") {
  GIVEN("an object with no materials listed") {
    useData(R"(
      <objectType id="rock" />
    )");

    WHEN("a user tries to build one") {
      client->sendMessage(CL_CONSTRUCT, makeArgs("rock", 10, 15));
      REPEAT_FOR_MS(100);

      THEN("there are still no objects") {
        CHECK(server->entities().size() == 0);
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Construction tools",
                 "[construction][tool]") {
  GIVEN("an object that needs a tool to be constructed") {
    useData(R"(
        <item id="circuitboard" />
        <item id="screwdriver" >
            <tag name="screwdriver" />
        </item>
        <objectType
            id="computer" constructionTime="0" constructionReq="screwdriver" >
            <material id="circuitboard" />
        </objectType>
      )");

    WHEN("a user tries to construct it") {
      client->sendMessage(CL_CONSTRUCT, makeArgs("computer", 10, 15));

      THEN("he gets a warning") {
        CHECK(client->waitForMessage(WARNING_NEED_TOOLS));

        AND_THEN("no object was created") {
          CHECK(server->entities().size() == 0);
        }
      }
    }
  }

  GIVEN("a 200ms construction that requires a tool, and a double-speed tool") {
    useData(R"(
        <objectType
          id="fire" constructionTime="200" constructionReq="fireLighting" >
          <material id="wood" />
        </objectType>
        <item id="matches" >
            <tag name="fireLighting" toolSpeed="2" />
        </item>
        <item id="wood" />
      )");

    AND_GIVEN("a user has the tool") {
      user->giveItem(&server->getFirstItem());

      WHEN("he tries to construct the object") {
        client->sendMessage(CL_CONSTRUCT, makeArgs("fire", 10, 15));

        AND_WHEN("150ms elapses") {
          REPEAT_FOR_MS(150);

          THEN("there is an object") { CHECK(server->entities().size() == 1); }
        }
      }
    }
  }
}

TEST_CASE("Construction progress is persistent",
          "[construction][persistence]") {
  // Given a brick wall with no materials added, owned by Alice
  auto data = R"(
    <objectType
      id="wall" constructionTime="0" >
      <material id="brick" />
    </objectType>
    <item id="brick" />
  )";
  {
    auto s = TestServer::WithDataString(data);
    s.addObject("wall", {10, 15}, "Alice");

    // And Alice has a brick
    auto c = TestClient::WithUsernameAndDataString("Alice", data);
    s.waitForUsers(1);
    auto &alice = s.getFirstUser();
    const auto *brick = &s.getFirstItem();
    alice.giveItem(brick);

    // When she adds it to the construction site
    const auto &wall = s.getFirstObject();
    c.sendMessage(CL_SWAP_ITEMS,
                  makeArgs(Serial::Inventory(), 0, wall.serial(), 0));

    // And when the construction finishes
    WAIT_UNTIL(!wall.isBeingBuilt());

    // And when the server restarts
  }
  {
    auto s = TestServer::WithDataStringAndKeepingOldData(data);

    // Then the wall is still complete
    WAIT_UNTIL(s.entities().size() == 1);
    const auto &wall = s.getFirstObject();
    CHECK(!wall.isBeingBuilt());
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "A construction material can 'return' an item",
                 "[construction][inventory]") {
  GIVEN("matches are needed to build a fire, and return a matchbox") {
    useData(R"(
      <objectType id="fire" constructionTime="0" >
        <material id="matches" returns="matchbox" />
      </objectType>
      <item id="matches" returnsOnConstruction="matchbox" />
      <item id="matchbox" />
    )");

    AND_GIVEN("a user has matches") {
      auto &matches = *server->items().find(ServerItem{"matches"});
      user->giveItem(&matches);
      WAIT_UNTIL(client->inventory()[0].first.type() != nullptr);

      WHEN("the user builds a fire") {
        client->sendMessage(CL_CONSTRUCT, makeArgs("fire", 10, 15));
        WAIT_UNTIL(server->entities().size() == 1);
        const Object &fire = server->getFirstObject();

        THEN("it is under construction") { CHECK(fire.isBeingBuilt()); }

        AND_WHEN("he adds his matches") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                      fire.serial(), 0));
          REPEAT_FOR_MS(100);

          THEN("he has a matchbox") {
            const auto &item = user->inventory(0);
            CHECK(item.type()->id() == "matchbox");
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Return-on-construction with full inventory",
                 "[construction][inventory]") {
  GIVEN("stackable ice cubes build an igloo and return an ice-cube tray") {
    useData(R"(
      <objectType id="igloo" constructionTime="0" >
        <material id="ice" />
      </objectType>
      <item id="ice" stackSize="10" returnsOnConstruction="iceCubeTray" />
      <item id="iceCubeTray" />
    )");

    AND_GIVEN("a user has an inventory full of ice") {
      auto &ice = server->findItem("ice");
      user->giveItem(&ice, 10 * User::INVENTORY_SIZE);

      WHEN("the user builds an igloo") {
        client->sendMessage(CL_CONSTRUCT, makeArgs("igloo", 10, 15));
        WAIT_UNTIL(server->entities().size() == 1);
        const Object &igloo = server->getFirstObject();

        AND_WHEN("he tries to add ice (swap)") {
          client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                      igloo.serial(), 0));

          THEN("the igloo is still unfinished") {
            REPEAT_FOR_MS(100);
            CHECK(igloo.isBeingBuilt());
          }
        }

        AND_WHEN("he tries to add ice (auto-construct)") {
          client->sendMessage(CL_AUTO_CONSTRUCT, makeArgs(igloo.serial()));

          THEN("the igloo is still unfinished") {
            REPEAT_FOR_MS(100);
            CHECK(igloo.isBeingBuilt());
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Auto-fill",
                 "[construction][inventory]") {
  GIVEN("an object requiring an item") {
    useData(R"(
      <objectType id="trap" constructionTime="0" >
        <material id="meat" />
      </objectType>
      <item id="meat" />
      <item id="gold" />
    )");

    auto &trap = server->addObject("trap", {10, 15});

    THEN("a user can see that it needs the item") {
      WAIT_UNTIL(client->objects().size() == 1);
      const auto &cTrap = client->getFirstObject();
      WAIT_UNTIL(cTrap.isBeingConstructed());

      AND_GIVEN("he has the required item") {
        auto &meat = server->findItem("meat");
        user->giveItem(&meat);

        WHEN("he auto-fills") {
          client->sendMessage(CL_AUTO_CONSTRUCT, makeArgs(trap.serial()));

          THEN("the building is complete") {
            WAIT_UNTIL(!trap.isBeingBuilt());

            AND_THEN("the player knows") {
              WAIT_UNTIL(!cTrap.isBeingConstructed());
            }
          }
        }
      }

      AND_GIVEN("the user has no item") {
        WHEN("he auto-fills") {
          client->sendMessage(CL_AUTO_CONSTRUCT, makeArgs(trap.serial()));

          THEN("the building is still incomplete") {
            REPEAT_FOR_MS(100);
            CHECK(trap.isBeingBuilt());
          }
        }
      }

      AND_GIVEN("the user has the wrong item") {
        auto &gold = server->findItem("gold");
        user->giveItem(&gold);

        WHEN("he auto-fills") {
          client->sendMessage(CL_AUTO_CONSTRUCT, makeArgs(trap.serial()));

          THEN("the building is still incomplete") {
            REPEAT_FOR_MS(100);
            CHECK(trap.isBeingBuilt());
          }
        }
      }
    }
  }

  GIVEN("a plant requiring dirt and a seed") {
    useData(R"(
      <objectType id="plant" constructionTime="0" >
        <material id="dirt" />
        <material id="seed" />
      </objectType>
      <item id="dirt" />
      <item id="seed" />
    )");

    auto &plant = server->addObject("plant", {10, 15});

    AND_GIVEN("a user") {
      WAIT_UNTIL(client->objects().size() == 1);

      AND_GIVEN("he has dirt") {
        auto &dirt = server->findItem("dirt");
        user->giveItem(&dirt);

        WHEN("he auto-fills") {
          client->sendMessage(CL_AUTO_CONSTRUCT, makeArgs(plant.serial()));

          THEN("the plant is still incomplete") {
            REPEAT_FOR_MS(100);
            CHECK(plant.isBeingBuilt());
          }
        }

        AND_GIVEN("he also has a seed") {
          auto &seed = server->findItem("seed");
          user->giveItem(&seed);

          WHEN("he auto-fills") {
            client->sendMessage(CL_AUTO_CONSTRUCT, makeArgs(plant.serial()));

            THEN("the plant is complete") { WAIT_UNTIL(!plant.isBeingBuilt()); }
          }
        }
      }
    }
  }

  WHEN("a user auto-fills with a bad serial") {
    const auto BAD_SERIAL = "42";
    client->sendMessage(CL_AUTO_CONSTRUCT, BAD_SERIAL);

    THEN("the server survives") { server->nop(); }
  }
}
