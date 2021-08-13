#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Construction materials can be added", "[construction]") {
  GIVEN("a wall that requires a brick") {
    auto data = R"(
      <objectType
        id="wall" constructionTime="0" >
        <material id="brick" quantity="1" />
      </objectType>
      <item id="brick" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    AND_GIVEN("a user has a brick") {
      s.waitForUsers(1);
      User &user = s.getFirstUser();
      user.giveItem(&*s.items().begin());

      WHEN("he starts building a wall") {
        c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 15));
        WAIT_UNTIL(s.entities().size() == 1);
        const Object &wall = s.getFirstObject();

        THEN("it needs materials") { CHECK(wall.isBeingBuilt()); }

        AND_WHEN("he gives adds a brick") {
          c.sendMessage(CL_SWAP_ITEMS,
                        makeArgs(Serial::Inventory(), 0, wall.serial(), 0));

          THEN("construction has finished") {
            WAIT_UNTIL(!wall.isBeingBuilt());
          }
        }
      }
    }
  }
}

TEST_CASE("Client knows about default constructions", "[construction]") {
  GIVEN("a wall that requires a brick, and has no unlocks specified") {
    auto data = R"(
      <objectType
        id="wall" constructionTime="0" >
        <material id="brick" quantity="1" />
      </objectType>
      <item id="brick" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a new user connects") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);

      THEN("he knows how to build one") { CHECK(c.knowsConstruction("wall")); }
    }
  }
}

TEST_CASE("New client doesn't know any locked constructions",
          "[construction]") {
  GIVEN("a constructable table with an unlock") {
    auto data = R"(
      <objectType
        id="table" constructionTime="0" >
        <material id="wood" quantity="5" />
        <unlockedBy item="wood" />
      </objectType>
      <item id="wood" />
    )";
    auto s = TestServer::WithDataString(data);

    WHEN("a new user connects") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);

      THEN("he doesn't know it") { CHECK_FALSE(c.knowsConstruction("table")); }
    }
  }
}

TEST_CASE("Unique objects are unique", "[construction]") {
  GIVEN("a buildable, unique object") {
    auto data = R"(
      <objectType id="throne" constructionTime="0" isUnique="1" >
        <material id="gold" quantity="1" />
      </objectType>
      <item id="gold" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    WHEN("a user builds one") {
      c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 15));
      WAIT_UNTIL(s.entities().size() == 1);

      AND_WHEN("he tries to build another one") {
        c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 15, 10));

        THEN("he gets a warning") {
          bool isConstructionRejected = c.waitForMessage(WARNING_UNIQUE_OBJECT);
          CHECK(isConstructionRejected);

          AND_THEN("there is still only the one object") {
            CHECK(s.entities().size() == 1);
          }
        }
      }
    }
  }
}

TEST_CASE("Constructing invalid object fails gracefully", "[construction]") {
  GIVEN("a user") {
    TestServer s;
    TestClient c;
    s.waitForUsers(1);

    WHEN("he tries to build a nonexistent object") {
      c.sendMessage(CL_CONSTRUCT, makeArgs("notARealObject", 10, 15));

      THEN("the server survives") {
        REPEAT_FOR_MS(100);
        s.nop();
      }
    }
  }
}

TEST_CASE("Objects can be unbuildable", "[construction]") {
  GIVEN("an object with materials, but marked as \"unbuildable\"") {
    auto data = R"(
    <objectType id="treehouse" constructionTime="0" isUnbuildable="1" >
      <material id="wood" quantity="10" />
    </objectType>
    <item id="wood" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    WHEN("a user tries to build one") {
      c.sendMessage(CL_CONSTRUCT, makeArgs("treehouse", 10, 15));
      REPEAT_FOR_MS(100);

      THEN("there are still no objects") { CHECK(s.entities().size() == 0); }
    }
  }
}

TEST_CASE("Clients can't see unbuildable constructions", "[construction]") {
  GIVEN("an object with materials, but marked as \"unbuildable\"") {
    auto data = R"(
    <objectType id="treehouse" constructionTime="0" isUnbuildable="1" >
      <material id="wood" quantity="10" />
    </objectType>
    <item id="wood" />
    )";
    auto s = TestServer::WithDataString(data);

    AND_GIVEN("a user") {
      auto c = TestClient::WithDataString(data);
      s.waitForUsers(1);

      THEN("he can't see it as a construction option") {
        CHECK_FALSE(c.knowsConstruction("treehouse"));
      }
    }
  }
}

TEST_CASE("Object types without materials can't be built", "[construction]") {
  GIVEN("an object with no materials listed") {
    auto data = R"(
      <objectType id="rock" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);
    s.waitForUsers(1);

    WHEN("a user tries to build one") {
      c.sendMessage(CL_CONSTRUCT, makeArgs("rock", 10, 15));
      REPEAT_FOR_MS(100);

      THEN("there are still no objects") { CHECK(s.entities().size() == 0); }
    }
  }
}

TEST_CASE("Construction tools", "[construction][tool]") {
  GIVEN("an object that needs a tool to be constructed") {
    auto data = R"(
        <item id="circuitboard" />
        <item id="screwdriver" >
            <tag name="screwdriver" />
        </item>
        <objectType
            id="computer" constructionTime="0" constructionReq="screwdriver" >
            <material id="circuitboard" />
        </objectType>
      )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    WHEN("a user tries to construct it") {
      s.waitForUsers(1);
      c.sendMessage(CL_CONSTRUCT, makeArgs("computer", 10, 15));

      THEN("he gets a warning") {
        CHECK(c.waitForMessage(WARNING_NEED_TOOLS));

        AND_THEN("no object was created") { CHECK(s.entities().size() == 0); }
      }
    }
  }

  GIVEN("a 200ms construction that requires a tool, and a double-speed tool") {
    auto data = R"(
        <objectType
          id="fire" constructionTime="200" constructionReq="fireLighting" >
          <material id="wood" />
        </objectType>
        <item id="matches" >
            <tag name="fireLighting" toolSpeed="2" />
        </item>
        <item id="wood" />
      )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    AND_GIVEN("a user has the tool") {
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      user.giveItem(&s.getFirstItem());

      WHEN("he tries to construct the object") {
        c.sendMessage(CL_CONSTRUCT, makeArgs("fire", 10, 15));

        AND_WHEN("150ms elapses") {
          REPEAT_FOR_MS(150);

          THEN("there is an object") { CHECK(s.entities().size() == 1); }
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

TEST_CASE("A construction material can 'return' an item",
          "[construction][inventory]") {
  GIVEN("matches are needed to build a fire, and return a matchbox") {
    auto data = R"(
      <objectType id="fire" constructionTime="0" >
        <material id="matches" returns="matchbox" />
      </objectType>
      <item id="matches" returnsOnConstruction="matchbox" />
      <item id="matchbox" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    AND_GIVEN("a user has matches") {
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      auto &matches = *s.items().find(ServerItem{"matches"});
      user.giveItem(&matches);
      WAIT_UNTIL(c.inventory()[0].first.type() != nullptr);

      WHEN("the user builds a fire") {
        c.sendMessage(CL_CONSTRUCT, makeArgs("fire", 10, 15));
        WAIT_UNTIL(s.entities().size() == 1);
        const Object &fire = s.getFirstObject();

        THEN("it is under construction") { CHECK(fire.isBeingBuilt()); }

        AND_WHEN("he adds his matches") {
          c.sendMessage(CL_SWAP_ITEMS,
                        makeArgs(Serial::Inventory(), 0, fire.serial(), 0));
          REPEAT_FOR_MS(100);

          THEN("he has a matchbox") {
            const auto &item = user.inventory(0);
            CHECK(item.type()->id() == "matchbox");
          }
        }
      }
    }
  }
}

TEST_CASE("Stackable items that build and return",
          "[construction][inventory]") {
  GIVEN("stackable ice cubes build an igloo and return an ice-cube tray") {
    auto data = R"(
      <objectType id="igloo" constructionTime="0" >
        <material id="ice" />
      </objectType>
      <item id="ice" stackSize="10" returnsOnConstruction="iceCubeTray" />
      <item id="iceCubeTray" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    AND_GIVEN("a user has an inventory full of ice") {
      s.waitForUsers(1);
      auto &user = s.getFirstUser();
      auto &ice = s.findItem("ice");
      user.giveItem(&ice, 10 * User::INVENTORY_SIZE);

      WHEN("the user builds an igloo") {
        c.sendMessage(CL_CONSTRUCT, makeArgs("igloo", 10, 15));
        WAIT_UNTIL(s.entities().size() == 1);
        const Object &igloo = s.getFirstObject();

        AND_WHEN("he tries to add ice") {
          c.sendMessage(CL_SWAP_ITEMS,
                        makeArgs(Serial::Inventory(), 0, igloo.serial(), 0));

          THEN("the igloo is still unfinished") {
            REPEAT_FOR_MS(100);
            CHECK(igloo.isBeingBuilt());
          }
        }
      }
    }
  }
}

TEST_CASE("Auto-fill", "[construction][inventory]") {
  GIVEN("an object requiring an item") {
    auto data = R"(
      <objectType id="trap" constructionTime="0" >
        <material id="meat" />
      </objectType>
      <item id="meat" />
      <item id="gold" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    auto &trap = s.addObject("trap", {10, 15});

    THEN("a user can see that it needs the item") {
      s.waitForUsers(1);
      WAIT_UNTIL(c.objects().size() == 1);
      auto &user = s.getFirstUser();
      const auto &cTrap = c.getFirstObject();
      WAIT_UNTIL(cTrap.isBeingConstructed());

      AND_GIVEN("he has the required item") {
        auto &meat = s.findItem("meat");
        user.giveItem(&meat);

        WHEN("he auto-fills") {
          c.sendMessage(CL_AUTO_CONSTRUCT, makeArgs(trap.serial()));

          THEN("the building is complete") {
            WAIT_UNTIL(!trap.isBeingBuilt());

            AND_THEN("the player knows") {
              WAIT_UNTIL(!cTrap.isBeingConstructed());
            }
          }
        }
      }

      AND_GIVEN("a user has no item") {
        WHEN("he auto-fills") {
          c.sendMessage(CL_AUTO_CONSTRUCT, makeArgs(trap.serial()));

          THEN("the building is still incomplete") {
            REPEAT_FOR_MS(100);
            CHECK(trap.isBeingBuilt());
          }
        }
      }

      AND_GIVEN("a user has the wrong item") {
        auto &gold = s.findItem("gold");
        user.giveItem(&gold);

        WHEN("he auto-fills") {
          c.sendMessage(CL_AUTO_CONSTRUCT, makeArgs(trap.serial()));

          THEN("the building is still incomplete") {
            REPEAT_FOR_MS(100);
            CHECK(trap.isBeingBuilt());
          }
        }
      }
    }
  }

  GIVEN("a plant requiring dirt and a seed") {
    auto data = R"(
      <objectType id="plant" constructionTime="0" >
        <material id="dirt" />
        <material id="seed" />
      </objectType>
      <item id="dirt" />
      <item id="seed" />
    )";
    auto s = TestServer::WithDataString(data);
    auto c = TestClient::WithDataString(data);

    auto &plant = s.addObject("plant", {10, 15});

    AND_GIVEN("a user") {
      s.waitForUsers(1);
      WAIT_UNTIL(c.objects().size() == 1);
      auto &user = s.getFirstUser();

      AND_GIVEN("he has dirt") {
        auto &dirt = s.findItem("dirt");
        user.giveItem(&dirt);

        WHEN("he auto-fills") {
          c.sendMessage(CL_AUTO_CONSTRUCT, makeArgs(plant.serial()));

          THEN("the plant is still incomplete") {
            REPEAT_FOR_MS(100);
            CHECK(plant.isBeingBuilt());
          }
        }

        AND_GIVEN("he also has a seed") {
          auto &seed = s.findItem("seed");
          user.giveItem(&seed);

          WHEN("he auto-fills") {
            c.sendMessage(CL_AUTO_CONSTRUCT, makeArgs(plant.serial()));

            THEN("the plant is complete") { WAIT_UNTIL(!plant.isBeingBuilt()); }
          }
        }
      }
    }
  }

  GIVEN("a user") {
    auto s = TestServer{};
    auto c = TestClient{};
    s.waitForUsers(1);

    WHEN("he auto-fills with a bad serial") {
      const auto BAD_SERIAL = "42";
      c.sendMessage(CL_AUTO_CONSTRUCT, BAD_SERIAL);

      THEN("the server survives") { s.nop(); }
    }
  }
}

TEST_CASE("Objects destroyed when used as tools", "[construction][tool]") {
  GIVEN("a rock that is destroyed when used to build an anvil") {
    auto data = R"(
      <objectType id="anvil" constructionTime="0" constructionReq="rock">
        <material id="iron" />
      </objectType>
      <objectType id="rock" destroyIfUsedAsTool="1" >
        <tag name="rock" />
      </objectType>
      <item id="iron" />
    )";
    auto s = TestServer::WithDataString(data);

    AND_GIVEN("a rock") {
      const auto &rock = s.addObject("rock", {10, 15});

      AND_GIVEN("a user") {
        auto c = TestClient::WithDataString(data);
        s.waitForUsers(1);

        WHEN("he builds an anvil") {
          c.sendMessage(CL_CONSTRUCT, makeArgs("anvil", 10, 5));

          THEN("the rock is dead") { WAIT_UNTIL(rock.isDead()); }
        }
      }
    }
  }
}
