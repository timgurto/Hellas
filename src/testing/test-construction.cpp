#include "TestClient.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE("Construction materials can be added") {
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
        c.sendMessage(CL_CONSTRUCT, makeArgs("wall", 10, 10));
        WAIT_UNTIL(s.entities().size() == 1);

        AND_WHEN("he gives adds a brick") {
          const Object &wall = s.getFirstObject();
          c.sendMessage(CL_SWAP_ITEMS,
                        makeArgs(Server::INVENTORY, 0, wall.serial(), 0));

          THEN("construction has finished") {
            WAIT_UNTIL(!wall.isBeingBuilt());
          }
        }
      }
    }
  }
}

TEST_CASE("Client knows about default constructions") {
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

TEST_CASE("New client doesn't know any locked constructions") {
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

TEST_CASE("Unique objects are unique") {
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
      c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));
      WAIT_UNTIL(s.entities().size() == 1);

      AND_WHEN("he tries to build another one") {
        c.sendMessage(CL_CONSTRUCT, makeArgs("throne", 10, 10));

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

TEST_CASE("Constructing invalid object fails gracefully") {
  GIVEN("a user") {
    TestServer s;
    TestClient c;
    s.waitForUsers(1);

    WHEN("he tries to build a nonexistent object") {
      c.sendMessage(CL_CONSTRUCT, makeArgs("notARealObject", 10, 10));

      THEN("the server survives") {
        REPEAT_FOR_MS(100);
        s.nop();
      }
    }
  }
}

TEST_CASE("Objects can be unbuildable") {
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
      c.sendMessage(CL_CONSTRUCT, makeArgs("treehouse", 10, 10));
      REPEAT_FOR_MS(100);

      THEN("there are still no objects") { CHECK(s.entities().size() == 0); }
    }
  }
}

TEST_CASE("Clients can't see unbuildable constructions") {
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

TEST_CASE("Objects without materials can't be built") {
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

TEST_CASE("Construction tools") {
  GIVEN("an object that needs a tool to be constructed") {
    auto data = R"(
        <item id="circuitboard" />
        <item id="screwdriver" >
            <tag name="screwdriver" />
        </item>
        <objectType
            id="computer" constructionTime="0" constructionReq="screwdriver" >
            <material id="circuitboard" quantity="1" />
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

TEST_CASE("Construction progress is persistent", "[persistence]") {
  // Given a brick wall with no materials added, owned by Alice
  auto data = R"(
    <objectType
      id="wall" constructionTime="0" >
      <material id="brick" quantity="1" />
    </objectType>
    <item id="brick" />
  )";
  {
    auto s = TestServer::WithDataString(data);
    s.addObject("wall", {10, 10}, "Alice");

    // And Alice has a brick
    auto c = TestClient::WithUsernameAndDataString("Alice", data);
    s.waitForUsers(1);
    auto &alice = s.getFirstUser();
    const auto *brick = &s.getFirstItem();
    alice.giveItem(brick);

    // When she adds it to the construction site
    const auto &wall = s.getFirstObject();
    c.sendMessage(CL_SWAP_ITEMS,
                  makeArgs(Server::INVENTORY, 0, wall.serial(), 0));

    // And when the construction finishes
    WAIT_UNTIL(!wall.isBeingBuilt());

    // And when the server restarts
  }
  {
    auto s = TestServer::WithDataString(data);

    // Then the wall is still complete
    REPEAT_FOR_MS(100);
    WAIT_UNTIL(s.entities().size() == 1);
    const auto &wall = s.getFirstObject();
    CHECK(!wall.isBeingBuilt());
  }
}

TEST_CASE("A construction material can 'return' an item") {
  GIVEN("matches are needed to build a fire, and return a matchbox") {
    auto data = R"(
      <objectType id="fire" constructionTime="0" >
        <material id="matches" quantity="1" returns="matchbox" />
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
        c.sendMessage(CL_CONSTRUCT, makeArgs("fire", 10, 10));
        WAIT_UNTIL(s.entities().size() == 1);

        AND_WHEN("he adds his matches") {
          const Object &fire = s.getFirstObject();
          c.sendMessage(CL_SWAP_ITEMS,
                        makeArgs(Server::INVENTORY, 0, fire.serial(), 0));

          THEN("he has a matchbox") {
            const auto &pItem = user.inventory()[0].first;
            WAIT_UNTIL(pItem.hasItem());
            CHECK(pItem.type()->id() == "matchbox");
          }
        }
      }
    }
  }
}
