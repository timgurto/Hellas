#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Scrapping items", "[scrapping]") {
  GIVEN("wood that can be scrapped into a woodchip") {
    useData(R"(
      <item id="wood" class="wood" />
      <item id="woodchip" />
      <itemClass id="wood">
        <canBeScrapped result="woodchip" />
      </itemClass>
    )");
    const auto &wood = server->findItem("wood");
    const auto &woodchip = server->findItem("woodchip");

    AND_GIVEN("the user has wood") {
      user->giveItem(&wood);

      WHEN("he scraps it") {
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

        THEN("he has a wood chip") {
          WAIT_UNTIL(user->inventory()[0].first.type() == &woodchip);
        }
      }
    }

    SECTION("Different slot") {
      AND_GIVEN("the user has wood in slot 5") {
        user->giveItem(&wood);
        client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                    Serial::Inventory(), 5));

        WHEN("he scraps it") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 5));

          THEN("he has a wood chip") {
            WAIT_UNTIL(user->inventory()[0].first.type() == &woodchip);
          }
        }
      }
    }

    SECTION("Correct slot is scrapped") {
      AND_GIVEN("the user has wood in inventory slots 0-2") {
        user->giveItem(&wood, 3);

        WHEN("he scraps the wood in slot 1") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 1));

          THEN("he has a wood chip in slot 1") {
            WAIT_UNTIL(user->inventory()[1].first.type() == &woodchip);
          }
        }
      }
    }
  }

  SECTION("Different items") {
    GIVEN("a rock that can be scrapped into sand") {
      useData(R"(
      <item id="rock" class="rock" />
      <item id="sand" />
      <itemClass id="rock">
        <canBeScrapped result="sand" />
      </itemClass>
    )");

      AND_GIVEN("the user has a rock") {
        const auto &rock = server->findItem("rock");
        user->giveItem(&rock);

        WHEN("he scraps it") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

          THEN("he has sand") {
            const auto &sand = server->findItem("sand");
            WAIT_UNTIL(user->inventory()[0].first.type() == &sand);
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData,
                 "Normal-distribution yield from scrapping", "[scrapping]") {
  GIVEN("wood can be scrapped into 1-3 woodchips") {
    useData(R"(
        <item id="wood" class="wood" stackSize="100" />
        <item id="woodchip" stackSize="1000" />
        <itemClass id="wood">
          <canBeScrapped result="woodchip" mean="2" sd="0.5" />
        </itemClass>
      )");

    AND_GIVEN("the user has 100 wood") {
      const auto &wood = server->findItem("wood");
      user->giveItem(&wood, 100);

      SECTION("Scrapping a stack only removes one item") {
        WHEN("he scraps one wood") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

          THEN("he has both wood and woodchips") {
            const auto &woodchip = server->findItem("woodchip");
            WAIT_UNTIL(user->inventory()[1].first.type() == &woodchip);
            CHECK(user->inventory()[0].first.type() == &wood);
          }
        }

        WHEN("he scraps all 100 wood") {
          for (auto i = 0; i != 100; ++i)
            client->sendMessage(CL_SCRAP_ITEM,
                                makeArgs(Serial::Inventory(), 0));

          THEN("he has no wood left") {
            WAIT_UNTIL(!user->inventory()[0].first.hasItem());
          }
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClientWithData, "Scrapping Equipped gear",
                 "[scrapping]") {
  GIVEN("a hat that can be scrapped into felt") {
    useData(R"(
      <item id="hat" class="hat" gearSlot="0" />
      <item id="felt" />
      <itemClass id="hat">
        <canBeScrapped result="felt" />
      </itemClass>
    )");

    AND_GIVEN("the user is wearing one") {
      const auto &hat = server->findItem("hat");
      user->giveItem(&hat);
      client->sendMessage(CL_SWAP_ITEMS,
                          makeArgs(Serial::Inventory(), 0, Serial::Gear(), 0));

      WHEN("he scraps it") {
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Gear(), 0));

        THEN("he has felt") {
          const auto &felt = server->findItem("felt");
          WAIT_UNTIL(user->inventory()[0].first.type() == &felt);
        }
      }
    }
  }
}

TEST_CASE_METHOD(ServerAndClient, "Scrapping with bad data", "[scrapping]") {
  SECTION("Empty slot") {
    client.sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));
    CHECK(client.waitForMessage(ERROR_EMPTY_SLOT));
  }

  SECTION("Invalid inventory slot") {
    client.sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 1000));
    CHECK(client.waitForMessage(ERROR_INVALID_SLOT));
  }

  SECTION("Invalid gear slot") {
    // Number of gear slots <= 9 < number of inventory slots
    client.sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Gear(), 9));
    CHECK(client.waitForMessage(ERROR_INVALID_SLOT));
  }
}

// TODO
// Container
// Container slot exceeding inventory size
// Object out of range
// Object no permission
// Not scrappable
// Bell curve
// Check inventory space
// Later: unlock repair skill
