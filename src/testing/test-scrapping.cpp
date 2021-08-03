#include "TestClient.h"
#include "TestFixtures.h"
#include "TestServer.h"
#include "testing.h"

TEST_CASE_METHOD(ServerAndClientWithData, "Scrapping", "[scrapping]") {
  GIVEN("wood that can be scrapped into a woodchip") {
    useData(R"(
      <item id="wood" class="wood" />
      <item id="woodchip" />
      <itemClass id="wood">
        <canBeScrapped result="woodchip" />
      </itemClass>
    )");

    AND_GIVEN("the user has wood") {
      const auto &wood = server->findItem("wood");
      user->giveItem(&wood);

      WHEN("he scraps it") {
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

        THEN("he has a wood chip") {
          const auto &woodchip = server->findItem("woodchip");
          WAIT_UNTIL(user->inventory()[0].first.type() == &woodchip);
        }
      }
    }

    SECTION("Different slot") {
      AND_GIVEN("the user has wood in slot 5") {
        const auto &wood = server->findItem("wood");
        user->giveItem(&wood);
        client->sendMessage(CL_SWAP_ITEMS, makeArgs(Serial::Inventory(), 0,
                                                    Serial::Inventory(), 5));

        WHEN("he scraps it") {
          client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 5));

          THEN("he has a wood chip") {
            const auto &woodchip = server->findItem("woodchip");
            WAIT_UNTIL(user->inventory()[0].first.type() == &woodchip);
          }
        }
      }
    }

    SECTION("Empty slot") {
      WHEN("the user tries scrapping an empty slot") {
        client->sendMessage(CL_SCRAP_ITEM, makeArgs(Serial::Inventory(), 0));

        THEN("he gets a warning") {
          CHECK(client->waitForMessage(ERROR_EMPTY_SLOT));
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

  SECTION("Nontrivial yield") {
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
            client->sendMessage(CL_SCRAP_ITEM,
                                makeArgs(Serial::Inventory(), 0));

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
}

// TODO
// Gear
// Container
// Empty slot
// Invalid slot number
// Object out of range
// Object no permission
// Not scrappable
// Bell curve
// Check inventory space
// Later: unlock repair skill
